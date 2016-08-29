# Pyramid Raider

A short game done in 72 hours for the Ludum Dare 36.

In a post-apocalyptic future, an explorer discovers a pyramid full of mysteries. Will he be able to save the world ? Probably not.


## Compilation

Clone the repository. We use a submodule to include our "game engine", Lair. So you should clone with `--recursive`:
```
git clone --recursive https://github.com/draklaw/<project_name>
```

The only dependency of the project is [Lair](https://github.com/draklaw/lair), our homebrew game engine. However, Lair has several dependency. Refer to Lair's readme for more info. Using a package manager is highly recommended. We build the Windows version using MSYS2.

We use cmake to compile. Once the dependencies are installed, assuming they are in a standard place where cmake can find them, all you need to do is
```
mkdir <path-to-some-build-directory>
cd <path-to-some-build-directory>
cmake <path-to-alice_hie>
make
```

If, as suggested above, you make an out-of-source build, you must make sure that the game can find the assets folder. Just copy or link the asset folder in the directory of the executable, and you're good to go. If the game complain about missing DLLs (typical under Windows), you have to copy them to the executable directory. Now enjoy the game !
