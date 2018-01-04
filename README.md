# grss

Progress Thru Processors (Grid Republic / Intel) screensaver for BOINC and GridRepublic

- this is a specialized screensaver for the BOINC/GridRepublic implementation ofthe Intel Progress Thru Processors
facebook initiative.

- there's just a few source files in src/ -- the ss_app.cpp is based on Dave's BOINC screensaver, and
   basically just sets up the usual BOINC graphics stuff, GUI RPC calls, and hands over rendering to....
   - grintelss.cpp --- the main file with all the calls for rendering the screensaver
     - this is all in a namespace "grint" for functions/globals needed to be called from ss_app.cpp

- note in the res/ directory the file "simt" -- this is actually res-backup/NeoSansIntelMedium.ttf
  - I'm not sure if we are OK for distributing this truetype file, so I at least renamed it so it's 
    not too obvious it's a truetype file!  eventually we should probably encrypt or put it in compiled form
    in a resource etc.

- obviously Mac libs & objs end up in mac_build or build, and Win stuff ends up in win_build

- for Mac, you can usually just run ./build.mac 
-    optional arguments:  demo   (builds a demo version)
                          all    (rebuilds all the libraries e..g freetype2 ftgl jpeg as universal libs)
-    XCode is setup to use the Development & Deployment configuration (not the other ones i.e. i386_Deployment etc)
-    important for Mac!  need to build newer libtool & libtoolize from ftp://gnu.org
       - do make install (/usr/local/lib) for this new libtool, then mv /usr/bin/libtool /usr/bin/ranlib
       (since ranlib is just a symlink to "old" libtool)
       - then ln -s /usr/local/bin/libtool /usr/bin/libtool to ensure the new libtool is used

- for Windows you need Visual Studio 2008 - just click on grss.sln
- Windows note -- GL/gl_aux.h & gl_aux.lib are deprecated - so remove the reference in boinc/api/boinc_gl.h to include gl_aux
- clean solutions between various builds (debug, release, win32, x64 etc)
- for final builds - set project to grss, Clean SOlution on the Release - Win32 and Release - x64 configs

- executables get placed in $HOME/Downloads on a Mac or in the project directory on Windows (name: grss.mac or grss.exe)
    (Mac it is also in the build/Development or build/Deployment directory)

- BOINC screensaver name should end up being boincscr for the Mac (a "Unix" exec) or boincscr.exe for Windows
   - directory /Library/Applicaton Support/BOINC Data for the Mac, could be anywhere on Windows!
   - files in res/* should get copied to the BOINC install folder

- it is assumed that the boinc directory is "parallel" to this directory, i.e. if this file is /home/user/grss 
  then boinc should be in /home/user/boinc

- all necessary libraries are included (except boinc)

- library versions used/tested/built:

     Freetype2  2.3.9  (freetype2
     FTGL 2.1.3~rc5    (freetype2 support for OpenGL)
     jpeglib 6b '98    (the old standard jpeg graphics lib from '98)

