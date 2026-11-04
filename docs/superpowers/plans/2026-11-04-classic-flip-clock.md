# Classic Flip Clock Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Deliver a classic black-and-white flip clock as an installable Windows screensaver.

**Architecture:** Native Win32 application with GDI+ rendering, portable C++ clock/argument/layout logic, and no third-party runtime. A standalone HTML visual companion allows previewing the style on the macOS build host.

**Tech Stack:** C++17, Win32, GDI+, MinGW-w64 cross compiler; HTML/CSS/JavaScript for the visual companion only.

**Spec:** User requested a Windows flip-clock screensaver and selected the classic style. Builder defaults: hours/minutes/seconds, 24-hour clock, optional seconds and date, independent clock on every monitor, slow positional drift, no network or sound. Target Windows 10/11 x64.

## Global Constraints
- Build in a new ClassicFlipClock directory; existing workspace projects are unrelated.
- Deliver an actual .scr binary and source, not only a browser clock.
- Support /s, /c[:HWND], /p HWND; invalid preview handles must never launch fullscreen.
- Mouse movement beyond a small threshold, mouse clicks and keyboard input exit fullscreen; preview ignores input.
- Configuration stored under HKCU only. Windows owns the idle timer and lock-on-resume policy.
- No Windows host is available; distinguish compile/portable tests from Windows runtime verification.

## Review Focus
- Midnight/noon, skipped seconds and time adjustments: formatted clock must converge immediately.
- Malformed command lines/overflowing parent handles must fail closed.
- Tiny preview panes, portrait screens and mixed-DPI displays must keep all cards visible.
- Preview destruction, display changes and exit must release all windows, timers and graphics resources.
- Installation must use a stable user-owned path and leave password/idle policy to Windows settings.

## Tasks
- [x] 1. Write and run failing portable C++ tests for argument parsing, clock formatting, responsive layout and flip phases; implement src/core.h; rerun full suite.
- [x] 2. Implement native renderer and Windows lifecycle/configuration. Cross-compile with warnings enabled, inspect PE architecture/imports. Native runtime acceptance is recorded separately.
- [x] 3. Create matching browser visual companion; inspect representative landscape/portrait views and animation.
- [ ] 4. Package .scr, installation helper, Chinese instructions, source/build scripts and checksums. Perform independent review, run all checks, and document remaining Windows manual checks.

## Execution record
User selected the offered “按经典款做” path, authorizing sensible defaults and direct execution. No additional design approval is required. This directory is a new standalone project, not an existing Git branch, so worktree/commit steps do not apply.

The user subsequently requested Git publication to `git@github.com:yocn/FlipClockScreenSaver.git`. Remote was empty; initialized main locally. Commit/push are now explicitly authorized.

- Portable test RED: `FAIL: no args opens settings`; GREEN: 53 checks passed.
- Cross compilation: MinGW-w64 14.0.0_3, `-Wall -Wextra -Werror`, successful PE32+ x86-64 GUI binary, only Windows system DLL imports.
- Browser: toggles, settings persistence, AM/PM/date label, flip start/settle, 1440×900 / 390×844 / 160×90 layouts pass; no JavaScript exceptions.
- Independent read-only reviewer found no blocker. Its DPI robustness recommendation was implemented with an explicit source/destination pixel rectangle for the final blit.
- Unsigned distribution, x64-only support and approximate HTML/native appearance are intentional scope choices, described in README.
- Windows CI includes native build, portable tests and lifecycle smoke checks. Interactive multi-monitor/DPI/input/idle/lock behavior remains a manual acceptance task.
