main: src/main.c
	cc src/main.c -lncurses -o noteapp -D DB_LOCATION=\"${HOME}/.notedb\"

install: 
	sudo mv noteapp /usr/bin/

clean:
	sudo rm -rf /usr/bin/noteapp