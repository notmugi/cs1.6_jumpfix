# Research notes: high-FPS landing slowdown

## Why command timing, not packet rate?

[ReGameDLL_CS issue 1029](https://github.com/rehlds/ReGameDLL_CS/issues/1029) points to [PR 1005](https://github.com/rehlds/ReGameDLL_CS/pull/1005). In `PM_WalkMove`, the post-jump stamina penalty multiplies horizontal velocity **once per ground movement step**. More steps in the same wall time means more damping. ReGameDLL normalizes the multiplier with an exponent based on frame time and `mp_stamina_restore_rate`. This repo does **not** port that formula or change server code: it changes the step sizes the client produces. One network packet may contain multiple commands; `cl_cmdrate` alone is not this fix.

## Where the prototype operates

Target inspected: native Steam Linux CS 1.6, protocol 48, `1.1.2.7/Stdio`, Oct 8 2024 build 10211. The original i386 ELF has symbols and debug info. Its build ID is `2eb3ff7acb804319d453335feed3be5ed21faab3`; use the **file hash** for identification, since the modified outputs retain the stock build-ID note. Addresses below are link-time virtual addresses, not runtime pointers.

- `_Host_Frame` calls `CL_Move` separately from `SCR_UpdateScreen`. Inside `CL_Move` the stock engine calls `ClientDLL_CreateMove` and **then** calculates `usercmd.msec` from `host_frametime` and a fractional remainder. [csldr's normal callback](https://github.com/mikkokko/csldr/blob/7d46c69/csldr/export.c#L16-L20) cannot suppress creation of that slot. Throttling mouse input was not the solution.
- Movement hook: `0x13a4ab`, after stock input sampling and timing calculation, before the duration store and `CL_PredictMove`. `stub.S` bridges the build-specific register/stack layout to `sample()` in `move.c`. A negative result exits this `CL_Move` pass without advancing the slot. A completed sample returns to `0x13a4b1` and the engine continues normally. Queued time, not a render-frame divisor, determines when a command is ready. This targets command **generation**, not guaranteed wire packet timing.
- Translation inputs are weighted by sample duration, buttons are ORed so a short press survives, and the last nonzero impulse and weapon selection are retained. Angles come from the latest sample. Multiple button edges or impulses in one window cannot all be represented. A long frame can produce a longer command rather than a burst of catch-up packets. Skipped passes also skip the remainder of `CL_Move`, including prediction and packet/voice work on that pass.
- Optional view hook: `V_RenderView` call at `0x169ca6`. `blend()` interpolates between predicted positions using queued milliseconds plus `frametime_remainder`. The stub temporarily replaces `ref_params.simorg` for the original `ClientDLL_CalcRefdef`, then restores it. No angle field or outgoing command is blended. This adds roughly one 10 ms step of positional presentation latency. It bypasses or resets for demos, pause, alternate views, long frames and large position jumps; see the guards for exact thresholds.
- `build.py` adds the compiled payload in padding after the RX segment and extends the RW zero-fill area for state. The full stock hash gates every layout assumption, including the saved stack slots. **For a new build**, re-derive the ordering, calling conventions, struct layouts, segment space and continuations before attempting a port.

## Evidence and remaining work

| Check | Result / limits |
|---|---|
| `test.c` | Synthetic one-second runs at each integer FPS from 100 to 1000 produce 100 commands totaling 1000 ms. Also covers a short impulse and a long frame. Not a real network test. |
| `verify.c` | Executes the staged x86 movement stub with stand-ins for engine continuations: 240 samples, 100 commands, one-frame tap retained. Smooth variant also exercises camera subframes, position restoration, teleport and pause. No live process attachment. |
| Playtest | Player reported the landing issue appeared fixed with movement-only, but translation visibly stepped. That motivated the smooth build. No controlled in-game measurements or user verdict on the smooth build recorded here. |

`local_alive` is shared by both hooks. It checks the current parsed local snapshot (`cl.frames[cl.parsecountmod].clientdata`) for `iuser1 == OBS_NONE`, `deadflag == DEAD_NO`, and positive health; an invalid frame or missing valid sequence disables the patch. ReGameDLL `UpdateClientData` preserves the observer’s `iuser1` even when substituting the target’s health in eye mode. The harness covers a healthy target, death, invalid frame and resuming without accumulated inputs or stale camera position. Still to verify in game: death → in-eye → chase → respawn.

Reference output SHA-256 (GCC 16.2.1, original development machine; toolchain changes may change these):

- Movement only: `59a5ef959e55b66a12e5418b6e985eb7f8150680d3098b3486407ad0925ad03f`
- Movement + smooth view: `51e451b7739426426f0efca987cd2b107a55d19b7182b9b894a252b3b86a702d`

Next useful comparison: same local map, same FPS and `cl_cmdrate`, stock vs movement-only vs smooth. Compare horizontal speed recovery after identical running jumps. Strafe past a wall and turn the mouse while stationary to distinguish positional cadence from look response. Check FPS fluctuating across 100, quick jump/fire and weapon switches, stairs, prediction corrections, map transitions, voice and demo playback. The current harness does not exercise these live paths. Do not assume secured-server compatibility from a local pass.
