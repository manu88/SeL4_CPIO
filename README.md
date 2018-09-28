# seL4 CPIO archive 

Uses projectPrepare script to setup the project

our root server program will be called `init`, and its only task will be to load an application from the CPIO  archive.

SeL4's build system has tools to create the CPIO archive.

##Project Preparation

Let's create an other application called `app`. As before, everything will be placed inside the project folder.

```
mkdir projects/app
mkdir projects/app/src 
```

Inside projects/app/src, we add the simpliest program :

```C
#include <stdio.h>

int main(void)
{
    printf("myapp started\n");
    return 0;
}
```

And we add a CMakeLists.txt inside projects/myapp :

```CMake
cmake_minimum_required(VERSION 3.7.2)

project(app C) # create a new C project called 'app' 


add_executable(app src/main.c) # add files to our project. Paths are relative to this file.

# This one is SUPER important, otherwise you will get an error 
# Caught cap fault in send phase at address
set_property(TARGET app APPEND_STRING PROPERTY LINK_FLAGS " -u __vsyscall_ptr ")

target_link_libraries(app sel4muslcsys  muslc) # we need to link against the standard C lib for printf
```

And we build. Everyting should build and run. Now, except for the new application, this is still a one-stupid application project. We need to create the CPIO archive file; then make our `init` task parse it and exec `myapp` application

## Create the CPIO Archive

SeL4's build system has a method nammed `MakeCPIO`. This utility can take a list of binairies and bundle them inside an object file.  Inside the init's CMakeLists.txt, simply add (before `add_executable`) :

```CMake
# list of apps to include in the cpio archive
get_property(cpio_apps GLOBAL PROPERTY apps_property)
MakeCPIO(archive.o "${cpio_apps}")
```

Then, the 'archive.o' has to be added to the executable binary :

```CMake
add_executable(init src/main.c archive.o)
```

This archive will be added to the init-image.

Now we have to add the custom app code to the list 'apps_property'. At the end of app's CMakeLists.txt:

```CMake
set_property(GLOBAL APPEND PROPERTY apps_property "$<TARGET_FILE:app>")
```

This will add our application to the list 'apps_property', to add to the CPIO archive.

### Note
With this method, we create a dependency between our application 'app' and init : init needs to know about app in order to create the CPIO archive. For now this only works because app's CMakeLists is executed _before_ init's (because of the alphabetical order). This would be different if 'app' were called 'zapp' !

## Access the CPIO archive from `init`

To access the CPIO archive, seL4's util_libs provides `libcpio`, so we need to link init against it. In init's CMakeLists.txt :

```CMake
target_link_libraries(init sel4muslcsys  muslc cpio) #  
``` 

In init's source code, we need to include some header from `libcpio`, and declare an external variable to hold the CPIO archive's data:

```C
#include <cpio/cpio.h>

/* The linker will link this symbol to the start address  *
 * of an archive of attached applications.                */
extern char _cpio_archive[];
```

Complete code to follow is here :
<https://github.com/manu88/SeL4_CPIO/blob/master/projects/init/src/main.c>

## Build and run
Just like the previous project:

```Bash
mkdir build
cd build
# configure
../init-build.sh  -DPLATFORM=x86_64 -DSIMULATION=TRUE
# make
ninja
#run (ctrl-A + X to quit qemu)
./simulate
```

