# Docker Images for ANaConDA

This directory contains Docker and Docker Compose files to build and run Docker images with various versions of ANaConDA running on specific operating systems. The `anaconda-base` folder contains a configurable Docker file for building Docker images with specific version of ANaConDA and specific operating system. Other directories then contain Docker Compose files which use the configurable Docker file to create (and run) containers with specific combinations of version of ANaConDA and operating system.

## Creating and running Docker images with ANaConDA

Use docker-compose. Pick a directory based on the desired version of ANaConDA and operating system and then run:

```
$ docker-compose up -d
```

For example, to create (and run) a container with Ubuntu 18.04 running latest stable version of ANaConDA and connect to it to perform analyses with ANaConDA, run the following:

```
$ cd anaconda-ubuntu18-latest
$ docker-compose up -d
$ docker exec -it anaconda-ubuntu18-latest bash
# Bash starts in the root directory of ANaConDA: ~/dev/projects/anaconda
# You can run 'tools/run.sh <analyser> <program>' to perform analysis
```

## Notes

The Docker Compose files use `anaconda-<os>` (e.g., `anaconda-ubuntu18`) as project name, therefore building containers for different versions of ANaConDA running on the same operating system may produce warnings during the creation. The containers with other versions of ANaConDA built by a different Docker Compose file will be viewed as orphan containers. This does not impact their functionality and multiple containers with different versions of ANaConDA running on the same operating system may exist concurrently without any issues.