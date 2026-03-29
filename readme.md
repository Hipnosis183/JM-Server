![](/assets/logo.svg)

## JM-Server

**JM-Server** is a server emulator for ***Jewelry Master***, an online arcade puzzle game developed by ***Arika*** in 2006; the original service ended around 2011. This server reimplementation allows you to play the game again, aiming to accurately replicate how the original server would've behaved.

There are two different implementations available, continue reading below to see which one you should use.

### Embedded Server

The **embedded** version is provided mainly for local/online play, although it can still be used to host large-scale servers. The server was written from the ground up in **C**, aiming for performance, portability and small size. The only dependencies are **LMDB** for the database and **MinHook** for patching the game.

This version allows to host a server, connect to a server, or both at the same time. It also works as a loader that takes care of all the modifications needed to make the game work with the custom server.

#### Setup

1. Download and unpack the server files in the game folder.
2. Optional: change the server settings inside `server.ini`.
3. Run `server.exe`.

At default settings, a server will get started on `localhost`, then the game client will open and connect to it automatically. When the game process finalizes, the server will also close.

#### Options

Options can be modified inside the file `server.ini`. Additional details can be found on the file itself.

- `ServerMode`: Defines the server behavior.
- `ServerHost`: Defines the server host.
- `Register`: Allows unregistered users to be registered at the login screen.
- `MultiScores`: Allows users to have mutiple scores/replays in the global rankings.
- `NoticeMode`: Defines the notice message text.

#### Build

To build the server files simply run `build.bat`; it'll generate the files `server.exe` and `server.dll`. Make sure to edit the script variable `gcc` to point to a valid 32-bit **GCC** install path. Additional compile flags are available:

- `-xp`: Builds a **Windows XP** compatible binary. Use the normal build for **Vista** onwards.
- `-debug`: Builds a debuggable binary, disabling all compiler optimizations.
- `-converter`: Builds a program that converts server data from an older version to be compatible with the current one. Also supports the `-debug` flag.

### General Server

The **general** version provides a more traditional web server approach, offering a robust, scalable and more stable alternative, making it more suitable to handle large number of connections and amounts of data. The server uses **Node.js** for the engine and **MongoDB** for the database.

This version only hosts the server. In order to play, the embedded server in client mode is required, as it also works as a loader/patcher.

#### Setup

1. Download and install **Node.js**, **npm** and **MongoDB**.
2. Install server dependencies with `npm install`.
3. Run the server with `node main`.

#### Options

Options can be modified inside the file `server/options.js`; requires a server restart.

- `Register`: Allows unregistered users to be registered at the login screen.
- `MultiScores`: Allows users to have mutiple scores/replays in the global rankings.

## Notes

- Replay files are not compatible between game versions (i.e. `1.32` -> `1.40`).