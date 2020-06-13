# setup

## Installing libcurl (to enable sending of audio file to the server)

Follow steps 1 to 5 in this answer taken from https://stackoverflow.com/a/54680718. In step 5, run `vcpkg.exe install curl[tool]` instead of the command specified in the answer:
![](https://github.com/denny-git/winform-setup/blob/master/install_libcurl.jpg)

## The hard part - linking the relevant files to the Windows Forms project and setting some options

Hopefully you haven't encountered any errors during the first step. Now we need to link some stuff to the project, or else it won't work properly.

### Including the relevant header files

To use curl, we need to include curl.h. Here's how you do it:

Right-click on MFCApp_Speech, then click "Properties" at the very bottom of the menu. Click on C/C++ first, then click "Additional include directories". Open the dropdown menu, then click "Edit". Click the displayed path and you should see a button with 3 dots appear to the right. Click the button, then navigate to the vcpkg install folder, which should look like this:

![](https://github.com/denny-git/winform-setup/blob/master/vcpkg_install_folder.jpg)

Go to `\packages\curl_x86_windows\include` and click "Select folder".

![](https://github.com/denny-git/winform-setup/blob/master/include_libcurl.jpg)

### Including the library file

After you have selected the additional include folder, go to Linker > General in the properties window. Edit the entry "Additional library directories" to point to `\packages\curl_x86_windows\lib` in the vcpkg install location.

![](https://github.com/denny-git/winform-setup/blob/master/include_curl_library.jpg)

### Adding preprocessor definitions

Go to C/C++ > Preprocessor Definitions. Add `"\_CRT_SECURE_NO_WARNINGS"` and `"CURL_STATICLIB"` to the preprocessor definitions.

![](https://github.com/denny-git/winform-setup/blob/master/definitions.jpg)

## Build the project

Build the project, and cross your fingers and hope it doesn't give any errors. If there aren't any errors, go ahead and run it.
