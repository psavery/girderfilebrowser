# Girder File Browser
A custom Qt file browser for browsing a girder server.

This is a stand-alone program that depends only on Qt, and it can be compiled and ran as such.

## Building
Dependencies:
  - Qt >= 5.5

To build, simply create a build directory, and then run `cmake <path/to/source/tree>`, and then `make`.
An executable will be placed in the build directory called `girderfilebrowser`.

## Authenticating the Girder File Browser
<img src="https://raw.githubusercontent.com/psavery/girderfilebrowser/master/images/currentLoginWindow.png" width="45%">

When the program is started, a login window will appear asking for the girder api url. This usually ends
with `/api/v1`. The username and password may then be entered. The password is sent using basic
http authentication, so it may be advisable to ensure that ssl is being used.

### Environment variables
Two environment variables are checked when the program starts: `GIRDER_API_URL` and `GIRDER_API_KEY`.
If `GIRDER_API_URL` exists, the api url on the login dialog will be pre-filled with its value.

If both `GIRDER_API_URL` and `GIRDER_API_KEY` exist, an automatic login via api key authentication
will be attempted. If successful, the girder file browser will immediately appear, and browsing
can begin. If api authentication fails, the login window will just appear.

## Running the Girder File Browser
<img src="https://raw.githubusercontent.com/psavery/girderfilebrowser/master/images/currentFileBrowser.png" width="35%">

The above image shows the current appearance of the file browser dialog.

## Using the Girder File Browser in an External Program
One potential use of the girder file browser is to have a user choose a file
on a girder server for some purpose in an application. For instance, the application
could have a user choose an input file on a girder server.
