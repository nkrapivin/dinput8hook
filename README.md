# dinput8hook

Forces DirectInput 8 games to read devices in background, whether they want it or not.

# Usage

The dlls are [here](https://github.com/nkrapivin/dinput8hook/releases).

For 32bit games, download `dinput8hook.dll`.

For 64bit games, download `dinput8hook_x64.dll`.

Then rename that dll into `dinput8.dll` and place into the same directory where the game executable is located.

Only tested with the GameMaker Studio 2.3 runner, both 32bit and 64bit.

If you have an Unreal Engine game then you might need to run the game once and use the Task Manager to find the real engine executable, since it is somewhere in the 'shipping' folder.

(that also applies to all games that use any launchers of sorts)

# Caution

This may not work properly with some games if the game engine's DirectInput code is uh, weird.

GameMaker Studio's code seems to be *fine*, just the flags are wrong. Your game might even crash.

# Building

Build the DLL in Visual Studio 2019 with C++.

The debug configuration will auto-wait till a debugger is attached to the game engine once the dll is called.

# Credits

- (YellowAfterlife)[https://yal.cc] - a funny screenshot and tips. (check out his extensions too)
- (me)[https://twitter.com/nkrapivindev] - hello!
