#!/bin/bash
docker run   --interactive --tty --rm  --env CONTAINER_TIMEOUT_SECONDS=1800  --env CODECLIMATE_CODE="$PWD"   --volume "$PWD":/code   --volume /var/run/docker.sock:/var/run/docker.sock   --volume /tmp/cc:/tmp/cc   codeclimate/codeclimate analyze -f html /code/src/ecs/test > /tmp/out.html

xdg-open /tmp/out.html
