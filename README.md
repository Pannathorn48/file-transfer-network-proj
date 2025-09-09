# Network Project
---
to run this project easily you can follow my instructions
1. source enviroment for this project
```bash
    source ./profile/activate
```
this will make this project can run easily but if you don't want to do it anymore you can do
```bash
    deactivate
```
this will delete all enviroment variable

2. observe `Makefile` after read Makefile you can run
```bash
    make build # to build whole project
```
this command will build whole project, alternatively you can run in (3.)

3. if you already did the first step you can run
```bash
    server # build and run server
```
or
```bash
    client # build and run client
```

4. if you've done everything you want you can
```bash
  deactivate
  make clear # (optional) for clear /bin
```
