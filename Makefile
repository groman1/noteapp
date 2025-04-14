main: src/main.c
	cc src/main.c -lncurses -o noteapp -D DB_LOCATION=\"${HOME}/.notedb\" -D TITLESIZE=24 -D ENTRYSIZE=96
	# if you need custom title/entry size, add -D TITLESIZE=... -D ENTRYSIZE=... in the end

install: 
	sudo mv noteapp /usr/bin/

clean:
	sudo rm -rf /usr/bin/noteapp

debug:
	cc src/main.c -lncurses -o noteapp-debug -D DB_LOCATION=\"${HOME}/.notedb\" -g
