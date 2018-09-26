# seL4 CPIO archive 

Uses projectPrepare script to setup the project

our root server program will be called `init`, and its only task will be to load an application from the CPIO  archive.

SeL4's build system has tools to create the CPIO archive.

##Project Preparation

Let's create an other application called `myApp`. As before, everything will be placed inside the project folder.

```
mkdir projects/myapp
mkdir projects/myapp/src 
```

Inside projects/myapp/src, we add the simpliest program :

```C
#include <stdio.h>

int main(void)
{
    printf("myapp started\n");
    return 0;
}
```

And we add a CMakeLists.txt inside projects/myapp :

```
cmake_minimum_required(VERSION 3.7.2)

project(myapp C) # create a new C project called 'Hello' 


add_executable(myapp src/main.c) # add files to our project. Paths are relative to this file.

target_link_libraries(myapp sel4muslcsys  muslc) # we need to link against the standard C lib for printf
```

And we build. Everyting should build and run. Now, except for the new application, this is still a one-stupid application project. We need to create the CPIO archive file; then make our `init` task parse it and exec `myapp` application

## Create the CPIO Archive

SeL4's build system has a method nammed `MakeCPIO`. This utility can take a list of binairies and bundle them inside an object file.  Inside the init's CMakeLists.txt, simply add (before `add_executable`) :

```
# list of apps to include in the cpio archive
get_property(cpio_apps GLOBAL PROPERTY apps_property)
MakeCPIO(archive.o "${cpio_apps}")
```

Then, the 'archive.o' has to be added to the executable binary :

```
add_executable(init src/main.c archive.o)
```

This archive will be added to the init-image.

Now we have to add the custom app code to the list 'apps_property'. At the end of app's CMakeLists.txt:

```
set_property(GLOBAL APPEND PROPERTY apps_property "$<TARGET_FILE:app>")
```

This will add our application to the list 'apps_property', to add to the CPIO archive.

### Note
With this method, we create a dependency between our application 'app' and init : init needs to know about app in order to create the CPIO archive. For now this only works because app's CMakeLists is executed _before_ init's (because of the alphabetical order). This would be different if 'app' were called 'zapp' !

## Access the CPIO archive from `init`

To access the CPIO archive, seL4's util_libs provides `libcpio`, so we need to link init against it. In init's CMakeLists.txt :

```
target_link_libraries(init sel4muslcsys  muslc cpio) #  
``` 

In init's source code, we need to include some header from `libcpio`, and declare an external variable to hold the CPIO archive's data:

```
#include <cpio/cpio.h>

/* The linker will link this symbol to the start address  *
 * of an archive of attached applications.                */
extern char _cpio_archive[];
```

Complete code to follow is here :
<https://github.com/manu88/SeL4_CPIO/blob/master/projects/init/src/main.c>