# PC tests: make test (host-jq.sh fetches jq and GFOpenRegex at jq.lock's commits)
CC ?= cc
test:
	sh test-part.sh
	mkdir -p ../build-host
	$(CC) -Wall -Wextra -I../amiga/include -I../amiga -o ../build-host/paths_test paths_test.c ../amiga/amiga_paths.c ../amiga/rx_names.c && ../build-host/paths_test
	sh host-jq.sh
.PHONY: test
