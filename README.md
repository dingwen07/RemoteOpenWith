# RemoteOpenWith
Open directly from your PC when clicking files in Remote PC

**WARNING**
This project isn't finished, running `server.py` will allow remote ACE on your computer. Currently it is just good for testing, kill the process immediately after you are done with it.

## How to use
1. Run `server.py` in your Physical PC (as for now you need Python, I will work on a Windows executable later and hopefully a POSIX one too).
2. In your remote PC, use `AssocFile.exe` to associate `RemoteOpenWith` with arbitrary file type (e.g. `.PDF`). For example, you can use command `AssocFile.exe RemoteOpenWith.AssocFile PDF <Path to RemoteOpenWith.exe>` to associate `RemoteOpenWith` with `.PDF` files.
3. In your remote PC, set `RemoteOpenWith` as the default program for the file types you want to open (you don't have to do the above step for every file type, `RemoteOpenWith` will show up if you expland the Open With dialog).
4. Make sure you are connected directly to your remote PC using port `3389`, if not, you have rebuild `RemoteOpenWith.exe`.
4. Click and go!

## How to build
1. Install Visual Studio with C++ and Windows SDK
2. Open a Developer Command Prompt/PowerShell for VS and navigate to the project folder
3. Run `cl .\RemoteOpenWith.c .\GetTCPConnections.c .\HashFile.c` to build `RemoteOpenWith.exe`, and `cl .\AssocFile.c` to build `AssocFile.exe`
4. There is another binary `CheckTCPConnections.exe` that you can build with `cl .\CheckTCPConnections.c .\GetTCPConnections.c`. This is a tool to inspect TCP connections by server port. You don't need it for this project, it is created to test functions in `GetTCPConnections.c`. 
