![](/assets/logo.svg)

## JM-Server

**JM-Server** is a server emulator for ***Jewelry Master***, an online arcade puzzle game developed by ***Arika*** in 2006. This project served as a gameplay test for the next game in the series, ***Jewelry Master Twinkle***. The original service ceased operations around 2011, and since then it remained unplayable, until now.

The server functionality has been fully reverse engineered and reimplemented in two different ways:

- A **NodeJS server**, mimicking how the original server worked, where people can connect to from the client over the internet.
- An **embedded C server**, as a portable solution that doesn't require any external server nor database initialization, while maintaining the full functionality and behaviour of the original.

Both implement complete user and rankings/leaderboards management, including replays storage. In addition, some options are available to further customise the server behaviour.

### NodeJS Server
The **NodeJS** version is meant to be used for hosting servers over the internet, although it can still be used to connect and play locally. As it uses proper server tech and a robust database engine (**MongoDB**), it's more suitable to handle multiple connections and big amounts of data.

To setup the server you'll need **Node**, **NPM** and **MongoDB Server**, and then install the dependencies with `npm install`. Use `node app.js` to run the server, or if you're on Windows, run the included `start.bat`, which also initializes the Mongo service. Replay files are stored in the server root directory, under the `rep` folder.

To change the server options, you'll have to modify the constants under `app.js` (connection) and `service.js` (users). To connect to a Node server from the client, add an entry in the *hosts file* redirecting `hg.arika.co.jp` to the server address, or read the section below.

### C Server
The **C** solution is provided for local and portable use, where by just running the executable, it initializes a minimal **Mongoose** embedded server with a light and performant **LMDB** database, at the same time it hooks the networking functions of the game to redirect the internal API calls to this local server.

This version doesn't require any dependencies, just place the files in the game folder and run the executable. The options can be changed in the `server.ini` file, but the default configuration is already the ideal for local play, and it will save both the database and replays under the `server` folder. These options are:

- **ServerMode**: Select the way the server is going to work.
- **HostName**: Select server address to connect to or host from.
- **HookDLL**: Enable/disable networking functions hooking.
- **Register**: Allow unregistered users to be registered at the login screen.
- **MultiScores**: Allow users to have mutiple scores (and replays) in the global rankings.
- **NoScores**: Disable scores and replays saving.

The different server modes allow you to play locally, connect to a server online, and host your own server over local or wide network. The modes affect the purpose of the `HostName` property value. All of this information can be found in detail inside the `server.ini` file. This also works as an alternative to modifying the *hosts file* manually.

### Building
To build the server I used **GCC** (**MinGW**), although any compiler will do with some extra configuration. The files can be compiled by running `build.bat`, make sure to point to a 32-bit GCC binary. It can also be compiled for 64-bit, but you'll need to replace the included libraries appropriately.