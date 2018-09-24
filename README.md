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

##Create the CPIO Archive

SeL4's build system has a method nammed `MakeCPIO`. Inside the main CMakeLists.txt, simply add :

```
MakeCPIO(archive.o "myapp") #We create an archive nammed archive.o containing myapp
```

This archive will be added to the init-image.

##Access the CPIO archive from `init`

