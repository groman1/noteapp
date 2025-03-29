#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <assert.h>
#include <string.h>

#define MAXENTRIES MAXY/4
#ifndef DB_LOCATION
#define DB_LOCATION ".notedb"
#endif

struct Entry
{
    char title[16];
    char entry[128];
    time_t time;
};

int entryCnt = 0;
int MAXX = 0, MAXY = 0;
FILE* dbFile;
struct Entry* db;

void highlightOption(int offset)
{
    attron(COLOR_PAIR(1));
    for (int i = 0; i<16; ++i)
    {
        if (mvinch(2+offset*4, i)==' ') continue;
        mvprintw(2+offset*4, i, "%c", mvinch(2+offset*4, i));
    }
    attroff(COLOR_PAIR(1));
}

void unhighlightOption(int offset)
{
    attron(COLOR_PAIR(2));
    for (int i = 0; i<16; ++i)
    {
        if (mvinch(2+offset*4, i)==' ') continue;
        mvprintw(2+offset*4, i, "%c", mvinch(2+offset*4, i));
    }
    attroff(COLOR_PAIR(2));
}

void initDb()
{
    db = malloc(sizeof(struct Entry));
    struct Entry *tempptr;
    entryCnt = 0;

    if(!(dbFile = fopen(DB_LOCATION, "r")))
    {
        dbFile = fopen(DB_LOCATION, "w");
        fclose(dbFile);
        dbFile = fopen(DB_LOCATION, "r");
    }

    int ch;
    int currValue = 0; // 0 - title, 1 - text, 2 - timestamp
    char currIndex = 0;
    int wasEscape = 0; // if the previous char was escape char (\)
    
    while ((ch = fgetc(dbFile))!=EOF)
    {
        switch (ch)
        {
            case '"': { 
                if (currValue) db[entryCnt].entry[currIndex] = wasEscape?'"':'\0';
                else db[entryCnt].title[currIndex] = wasEscape?'"':'\0';
                ++currValue;
                currIndex = -1;
                wasEscape = 0;
                break;
            }
            case '\\': { 
                if (!wasEscape) wasEscape = 1; 
                else 
                {
                    if (currValue) db[entryCnt].entry[currIndex] = '\\';
                    else db[entryCnt].title[currIndex] = '\\';
                    wasEscape = 0;
                }
                break;
            }
            case '\n': { wasEscape = 0; currIndex = -1; currValue = 0; ++entryCnt; while(!(tempptr = realloc(db, sizeof(struct Entry)*(entryCnt+1)))); db = tempptr; break; }
            default: {
                wasEscape = 0;
                switch (currValue)
                {
                    case 0: db[entryCnt].title[currIndex] = ch; break;
                    case 1: db[entryCnt].entry[currIndex] = ch; break;
                    case 2: db[entryCnt].time*=10; db[entryCnt].time+=ch-48; break;
                }
            }
        }
        ++currIndex;
    }
}

int contains(int* array, int length, int element)
{
    for (int i = 0; i < length; ++i)
    {
        if (array[i]==element) return 1;
    }
    return 0;
}

void printLine(int id, int offset)
{
    for (int i = 0; i<MAXX-2; ++i)
    {
        if (db[id].title[i]=='\0') break;
        mvprintw(2+offset*4, i,  "%c", db[id].title[i]);
    }
    for (int i = 0; i<MAXX-2; ++i)
    {
        if (db[id].entry[i]=='\0') break;
        mvprintw(3+offset*4, i,  "%c", db[id].entry[i]);
    }
    mvprintw(4+offset*4, 0, "%s", ctime(&(db[id].time)));
}

void printFound(int* showEntries, int offset, int length)
{
    clrtobot();
    if(length>0)
    {
        for (int i = offset; (i < MAXENTRIES+offset)&&(i<length); ++i)
        {
            printLine(showEntries[i], i-offset);
        }
    }
    else
    {
        mvprintw(3, 0, "No results found");
    }
}

char switchCases(char input)
{
    if (input>='a'&&input<='z')
    {
        return input-32;
    }
    else if (input>='A'&&input<='Z')
    {
        return input+32;
    }
    else
    {
        return input;
    }
}

int getFound(char title[16], int length, int* showEntries)
{
    clrtobot();
    int found = 1, qtyFound = 0;
    free(showEntries);
    showEntries = malloc(4);
    for (int i = 0; i < entryCnt; ++i)
    {
        for (int x = 0; x < 16; ++x)
        {
            if (db[i].title[x]==title[0]||switchCases(db[i].title[x])==title[0])
            {
                for (int o = 0; o < length; ++o)
                {
                    if ((db[i].title[x+o]!=title[o])&&(switchCases(db[i].title[x+o])!=title[o])) found = 0;
                }
                if (found&&!contains(showEntries, qtyFound, i))
                {
                    if (qtyFound>0) showEntries = realloc(showEntries, sizeof(int)*(qtyFound+1));
                    showEntries[qtyFound] = i;
                    ++qtyFound;
                }
                found = 1;
            }
        }
    }
    return qtyFound;
}

void rewriteDb()
{
    fclose(dbFile);
    assert(dbFile = fopen(DB_LOCATION, "w"));
    for (int e = 0; e < entryCnt; ++e)
    {
        for (int i = 0; i < 16; ++i)
        {
            if (db[e].title[i]=='\n')
            {
                fputc('\\', dbFile);
                fputc('n', dbFile);
            }
            else if (db[e].title[i]=='"')
            {
                fputc('\\', dbFile);
                fputc('"', dbFile);
            }
            else if (db[e].title[i]=='\0')
            {
                break;
            }
            else
            {
                fputc(db[e].title[i], dbFile);
            }
        }
        fputc('"', dbFile);
        for (int i = 0; i < 128; ++i)
        {
            if (db[e].entry[i]=='\n')
            {
                fputc('\\', dbFile);
                fputc('n', dbFile);
            }
            else if (db[e].entry[i]=='"')
            {
                fputc('\\', dbFile);
                fputc('"', dbFile);
            }
            else if (db[e].entry[i]=='\0')
            {
                break;
            }
            else 
            {
                fputc(db[e].entry[i], dbFile);
            }
        }
        fputc('"', dbFile);
        long divisor = 1000000000;
        while (divisor>0)
        {
            fputc((db[e].time/divisor%10)+48,dbFile);
            divisor/=10;
        }
        fputc('\n', dbFile);
    }
    fflush(dbFile);
}

void newOption(char title[16], char entry[128], time_t time)
{
    db = realloc(db, sizeof(struct Entry)*(entryCnt+1));
    for (int i = 0; i < 16; ++i)
    {
        if(title[i]=='\0') break;
        db[entryCnt].title[i] = title[i];
    }
    for (int i = 0; i < 128; ++i)
    {
        if(entry[i]=='\0') break;
        db[entryCnt].entry[i] = entry[i];
    }
    db[entryCnt].time = time;
    ++entryCnt;
    rewriteDb();
}

void newOptionEntry(char title[16], int posToReturn)
{
    clrtobot();
    int ch, currIndex = 0;
    char entry[128];
    mvprintw(3, 0, "Enter the text: ");
    while((ch=getch())!=10) //enter
    {
        switch (ch)
        {
            case 'a'...'z':
            case 'A'...'Z':
            case '0'...'9':
            case ' ': case '.': case ',': case '/': case '"': case ':': case '<': case '>': case '-': case '_': case '+': case '=': if(currIndex<128) { mvprintw(3, 16+currIndex, "%c", ch); entry[currIndex] = ch; ++currIndex; } break;
            case 263: if (currIndex>0) { entry[--currIndex] = ' '; mvprintw(3, 16+currIndex, " "); } break;
        }
        move(3, 16+currIndex);
    }
    entry[currIndex] = '\0';
    newOption(title, entry, time(NULL)); //add time here
    move(0, 23+posToReturn);
}

void editEntry(int id, int posToReturn)
{
    clrtobot();
    mvprintw(3,0, "Edit the text: %s", db[id].entry);
    int currLength = strlen(db[id].entry)-1;
    int ch;
    while((ch=getch())!=10) //enter
    {
        switch (ch)
        {
            case 'a'...'z':
            case 'A'...'Z':
            case '0'...'9':
            case ' ': case '.': case ',': case '/': case '"': case ':': case '<': case '>': case '-': case '_': case '+': case '=': if(currLength<128) { mvprintw(3, 16+currLength, "%c", ch); db[id].entry[++currLength] = ch; } break;
            case 263: if (currLength>0) { db[id].entry[currLength--] = ' '; mvprintw(3, 16+currLength, " "); } break;
        }
        move(3, 16+currLength);
    }
    db[id].entry[currLength+1] = '\0';
    db[id].time = time(NULL);
    rewriteDb();
    move(0, 23+posToReturn);
    clrtobot();
}

void browseEntry(int id, char searchText[16])
{
    clear();
    for (int i = 0; i<MAXX-2; ++i)
    {
        if (db[id].title[i]=='\0') break;
        mvprintw(0, i, "%c", db[id].title[i]);
    }
    for (int i = 0; i<MAXX-2; ++i)
    {
        if (db[id].entry[i]=='\0') break;
        mvprintw(1, i, "%c", db[id].entry[i]);
    }
    mvprintw(2, 0, "%s", ctime(&(db[id].time)));
    while(getch()!=263);
    clear();
    mvprintw(0,0,"Enter the search name: %s", searchText);
}

void removeOption(int id)
{
    while (id<entryCnt)
    {
        db[id] = db[++id];
    }
    db = realloc(db, (--entryCnt)*sizeof(struct Entry));
    rewriteDb();
}


int main()
{
    initscr();
    getmaxyx(stdscr, MAXY, MAXX);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);

    noecho();
    keypad(stdscr, 1);
    initDb();

    printw("Enter the search name: ");
    int ch;
    char title[16];
    int currLength = 0;
    int lenFound = 0, currSelected = 0; // currSelected - current selected
    int currOffset = 0; // quantity of entries scrolled; from what entry to start displaying
    int* entryIds = malloc(4);

    while(ch=getch())
    {
        switch (ch)
        {
            case 'a'...'z':
            case 'A'...'Z':
            case '0'...'9':
            case ' ': case '.': case ',': case '/': case '"': case ':': case '<': case '>': case '-': case '_': case '+': case '=': if(currLength<15) { currSelected = 0; currOffset = 0; mvprintw(0, 23+currLength, "%c", ch); title[currLength] = ch; ++currLength; lenFound = getFound(title, currLength, entryIds); printFound(entryIds, currOffset, lenFound); highlightOption(0); } break;
            case 263: if (currLength>1) { mvprintw(0, 23+(--currLength), " "); title[currLength] = '\0'; lenFound = getFound(title, currLength, entryIds); printFound(entryIds, currOffset, lenFound); highlightOption(0);  } else if (currLength==1) { title[currLength] = '\0'; --currLength; mvprintw(0, 23, " "); clrtobot(); } break;
            case 258: { if ((currSelected<MAXENTRIES-1)&&(currSelected<lenFound-1)) { unhighlightOption(currSelected); highlightOption(++currSelected); } else { if (currOffset<lenFound-MAXENTRIES) { ++currOffset; lenFound = getFound(title, currLength, entryIds); printFound(entryIds, currOffset, lenFound); highlightOption(currSelected); } } break; } 
            case 259: { if (currSelected>0) { unhighlightOption(currSelected); highlightOption(--currSelected); } else { if (currOffset>0) { --currOffset; lenFound = getFound(title, currLength, entryIds); printFound(entryIds, currOffset, lenFound); highlightOption(currOffset); } } break; }
            case 331: { newOptionEntry(title, currLength); lenFound = getFound(title, currLength, entryIds); printFound(entryIds, currOffset, lenFound); highlightOption(0); break; }
            case 10:  { if(lenFound) { browseEntry(entryIds[currSelected], title); lenFound = getFound(title, currLength, entryIds); printFound(entryIds, currOffset, lenFound); highlightOption(currSelected); } break; }
            case 330: { endwin(); return 0; }
            case 4: { removeOption(entryIds[currSelected]); currSelected = 0; currOffset = 0; lenFound = getFound(title, currLength, entryIds); printFound(entryIds, currOffset, lenFound); highlightOption(0); break; }
            case 5: { editEntry(currSelected, currLength); currSelected = 0; currOffset = 0; lenFound = getFound(title, currLength, entryIds); printFound(entryIds, currOffset, lenFound); highlightOption(0); break; }
            default:  break;
        }
        move(0, 23+currLength);
    }
    return 0;
}