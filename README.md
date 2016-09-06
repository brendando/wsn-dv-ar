Remember to run `git submodule update --init` in the contiki submodule.

Compiling binary for sensortags: `make TARGET=srf06-cc26xx BOARD=sensortag/cc2650`
Makefile runs in working directory. Make sure CONTIKI variable is referenced correctly relative to Makefile's working diectory.

To run server, `node main.js` after running `npm install`
