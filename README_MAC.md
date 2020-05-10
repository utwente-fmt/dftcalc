# Compilation on MacOS (tested on Catalina):

## Compiling DFTCalc

- Install XCode from the App Store to install the compiler etc.

- XCode ships with out-of-date versions of bison and flex, and does not
  include cmake and yaml-cpp. To install (current versions of) the
  dependencies:
  - Install [Homebrew](https://brew.sh/)
  - Run `brew install cmake yaml-cpp flex bison`
  - Follow Brew's instruction and put into `~/.zshrc`:
```
export PATH="/usr/local/opt/bison/bin:/usr/local/opt/flex/bin:$PATH"
```

- Compile and install as usual
```
mkdir build && cd build && cmake -DDFTROOT=/usr/local .. && make && sudo make install
```

## Compiling DFTRES

- Obtain [DFTRES](https://github.com/utwente-fmt/DFTRES)
- In DFTRES's folder, run `make jar`
- Add to your `~/.zshrc`:
```
export DFTRES="<path to DFTRES>"
```

## Install [Storm](https://stormchecker.org)

- Add the Storm homebrew /tap/ and install Storm
```
brew tap moves-rwth/storm
brew install stormchecker
```
