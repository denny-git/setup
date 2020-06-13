# PART I - Setting up the Windows Forms app

## Installing libcurl using vcpkg (to enable sending of audio file to the server)

Follow steps 1 to 5 in this answer taken from https://stackoverflow.com/a/54680718, albeit with the following changes:

* In step 2, the Visual Studio Developer Tools version should be 2019 and not 2017 (if you have installed Visual Studio 2019 on your laptop).

* In step 5, the command to run should be `vcpkg.exe install curl[tool]` instead of the one specified in the answer.

![](https://github.com/denny-git/winform-setup/blob/master/install_libcurl.jpg)

## The hard part - linking the relevant files to the Windows Forms project and setting some options

Hopefully you haven't encountered any errors during the first step. Now we need to link some stuff to the project, or else Visual Studio will complain as it doesn't know where to find them.

### Including the relevant header files

In order for the app to use libcurl to send audio to the server, we need to include curl.h in the project. Here's how you do it:

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

# PART II - Integration of web server into the speech-to-text library

## Adding httplib.h - the web server library
The speech-to-text library needs cpp-httplib in order to accept file uploads from users. Hence we need to add httplib.h to the project.

Navigate to `\student_nnet3v03lib01` (or whatever folder contains the sub-folders seen in the image below) in the speech-to-text library project folder.

![](https://github.com/denny-git/setup/blob/master/folder_to_add.jpg)

Add a subfolder named `http` to the above-mentioned folder, then download (httplib.h)[https://github.com/denny-git/setup/blob/master/httplib.h] and save it to the `http` folder.

## Including httplib.h in the project
We need to do this so that Visual Studio knows where the file is. Right-click on the project name, then click "Properties" at the very bottom of the menu. Click on C/C++ first, then click "Additional include directories". Open the dropdown menu, then click "Edit". Click the button to the left of the red "X" to add a new line. Click the button with 3 dots that appears to the right, navigate to the `http` folder created just now, and then click "Select folder".

![](https://github.com/denny-git/setup/blob/master/include_httplib.jpg)

## Replace content of nnet3_calling.cc
Replace the contents of nnet3_calling.cc in your project with the contents of (this file)[https://github.com/denny-git/setup/blob/master/nnet3_calling.cc]. Watch out for any errors highlighted by Visual Studio (hopefully there are none).

## Running the enhanced speech-to-text library
Now that we have added cpp-httplib to the code, run the project (and hope everything goes well). Wait for it to load everything first - the console should look like this when loading has completed:

![](https://github.com/denny-git/setup/blob/master/enhanced_kaldi_console.jpg)

## Testing out the speech-to-text server
Visit localhost:8080/file to access the wav file upload page to test the speech-to-text server. It should look like this:

![](https://github.com/denny-git/setup/blob/master/upload_page.jpg)

Choose a wav file that was supplied with the speech-to-text code in `\student_nnet3v03lib01\platform\300hrs\data\wav`, then click "Submit". This is an example of what should happen:

![](https://github.com/denny-git/setup/blob/master/text_from_wav_upload.jpg)

# Testing out the Windows Forms app with speech-to-text capability

## Prerequisites
Always ensure that the speech-to-text server is already running before you run the Windows Forms app.

## How to use
This is what the new UI looks like:

![](https://github.com/denny-git/setup/blob/master/winform.jpg)

Click "Normal Audio" to begin recording. When done, click it again to stop recording.

Audio is sent to the server for transcription immediately after you stop recording. The entire program will freeze for some time while waiting for a response from the speech-to-text server, which could take > 10 seconds. An example of a completed transcription is shown below (audio taken from a radio news broadcast).

![](https://github.com/denny-git/setup/blob/master/winforms_with_text.jpg)

Transcribed text appears in the box with the disabled scroll bar. The "Status" box and "Send audio" buttons currently do not do anything and may be ignored.
