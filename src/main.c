#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rawtui.h"
#include <string.h>

#define MAXENTRIES MAXY/4-((MAXY+1)%4>0)
#ifndef DB_LOCATION
#define DB_LOCATION ".notedb"
#endif

#ifndef TITLESIZE 
#define TITLESIZE 16
#endif

#ifndef ENTRYSIZE
#define ENTRYSIZE 128
#endif

struct Entry
{
    char title[TITLESIZE];
    char entry[ENTRYSIZE];
    time_t time;
};

int entryCnt = 0;
uint16_t MAXX = 0, MAXY = 0;
FILE* dbFile;
struct Entry* db;

void highlightOption(int offset, char *entryname)
{
    wrattr(COLORPAIR(1));
	moveprint(2+offset*4, 0, entryname);
	wrattr(NORMAL);
}

void unhighlightOption(int offset, char *entryname)
{
    wrattr(NORMAL);
	moveprint(2+offset*4, 0, entryname);
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
	fclose(dbFile);
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
	if (2+offset*4<MAXY) moveprintsize(2+offset*4, 0, db[id].title, strlen(db[id].title)>MAXX-2?MAXX-2:strlen(db[id].title));
	if (3+offset*4<MAXY) moveprintsize(3+offset*4, 0, db[id].entry, strlen(db[id].entry)>MAXX-2?MAXX-2:strlen(db[id].entry));
	if (4+offset*4<MAXY) moveprint(4+offset*4, 0, ctime(&(db[id].time)));
}

void printFound(int* showEntries, int offset, int length)
{
	move(2,0);
    cleartobot();
    if(length>0)
    {
        for (int i = offset; (i < MAXENTRIES+offset)&&(i<length); ++i)
        {
            printLine(showEntries[i], i-offset);
        }
    }
    else
    {
        moveprint(3, 0, "No results found");
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

int *getFound(char title[TITLESIZE], int length, int *qtyF)
{
    cleartobot();
    int found = 1, qtyFound = 0;
	int *showEntries;
	if (length)
	{
		showEntries = malloc(4);
		for (int i = 0; i < entryCnt; ++i)
		{
			for (int x = 0; x < strlen(db[i].title); ++x)
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
		*qtyF = qtyFound;
		return showEntries;
	}
	showEntries = malloc(4*entryCnt);
	for (int i = 0; i<entryCnt; ++i)
	{
		showEntries[i] = i;
	}
	*qtyF = entryCnt;
	return showEntries;
}

void rewriteDb()
{
    dbFile = fopen(DB_LOCATION, "w+");
    for (int e = 0; e < entryCnt; ++e)
    {
        for (int i = 0; i < TITLESIZE; ++i)
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
        for (int i = 0; i < ENTRYSIZE; ++i)
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
    fclose(dbFile);
}

void newOption(char title[TITLESIZE], char entry[ENTRYSIZE], time_t time)
{
    db = realloc(db, sizeof(struct Entry)*(entryCnt+1));
    for (int i = 0; i < TITLESIZE; ++i)
    {
        if(title[i]=='\0') break;
        db[entryCnt].title[i] = title[i];
    }
    for (int i = 0; i < ENTRYSIZE; ++i)
    {
        if(entry[i]=='\0') break;
        db[entryCnt].entry[i] = entry[i];
    }
    db[entryCnt].time = time;
    ++entryCnt;
    rewriteDb();
}

void newOptionEntry(char title[TITLESIZE], int posToReturn)
{
    cleartobot();
    char ch;
	int currIndex = 0;
    char entry[ENTRYSIZE];
    moveprint(3, 0, "Enter the text: ");
    while((ch=inesc())!=10&&ch!=13) //enter
    {
        switch (ch)
        {
			case 32 ... 126: if(currIndex<ENTRYSIZE) { moveprintsize(3, 16+currIndex, &ch, 1); entry[currIndex] = ch; ++currIndex; } break;
            case 127: if (currIndex>0) { entry[--currIndex] = ' '; moveprint(3, 16+currIndex, " "); } break;
        }
        move(3, 16+currIndex);
    }
    entry[currIndex] = '\0';
    newOption(title, entry, time(NULL)); //add time here
    move(0, 23+posToReturn);
}

void editEntry(int id, int posToReturn)
{
    cleartobot();
    moveprint(3,0, "Edit the text: ");
	moveprint(3, 15, db[id].entry);
    int currLength = strlen(db[id].entry);
    char ch;
    while((ch=inesc())!=10&&ch!=13) //enter
    {
        switch (ch)
        {
			case 32 ... 126: 
			{
				if(currLength<ENTRYSIZE-1) 
				{ 
					moveprintsize(3, 15+currLength, &ch, 1); 
					db[id].entry[currLength++] = ch; 
				}
				break;
			}
            case 127: 
			{
				if (currLength>0) 
				{ 
					db[id].entry[--currLength] = '\0'; 
					moveprint(3, 15+currLength, " "); 
				} 
				break;
			}
        }
        move(3, 15+currLength);
    }
    db[id].entry[currLength+1] = '\0';
    db[id].time = time(NULL);
    rewriteDb();
    move(0, 23+posToReturn);
    cleartobot();
}

void browseEntry(int id, char searchText[TITLESIZE])
{
    clear();
	setcursor(0);
	moveprintsize(0, 0, db[id].title, strlen(db[id].title)>MAXX-2?MAXX-2:strlen(db[id].title));
	moveprintsize(1, 0, db[id].entry, strlen(db[id].entry)>MAXX-2?MAXX-2:strlen(db[id].entry));
    moveprint(2, 0, ctime(&(db[id].time)));
    while(inesc()!=127);
	setcursor(1);
    clear();
    moveprint(0,0,"Enter the search name: ");
	print(searchText);
}

void removeOption(int id)
{
    while (id<entryCnt)
    {
        db[id] = db[id+1];
		++id;
    }
    db = realloc(db, (--entryCnt)*sizeof(struct Entry));
    rewriteDb();
}


int main()
{
    init();
    getTermXY(&MAXY,&MAXX);

    initcolorpair(1, BLACK, WHITE);

    initDb();

    moveprint(0,0,"Enter the search name: ");
    unsigned char ch;
    char title[TITLESIZE] = "";
    int currLength = 0;
    int lenFound = 0, currSelected = 0; // currSelected - current selected
    int currOffset = 0; // quantity of entries scrolled; from what entry to start displaying
    int* entryIds = malloc(4);
	entryIds = getFound(title, 0, &lenFound);
	printFound(entryIds, 0, lenFound);
	highlightOption(0, db[0].title);
	move(0, 23);
    while((ch=inesc()))
    {
        switch (ch)
		{
			case 32 ... 126: 
			{ 
				if(currLength<TITLESIZE-1) 
				{
					currSelected = 0; currOffset = 0; 
					moveprintsize(0, 23+currLength, &ch, 1); 
					title[currLength++] = ch; 
					entryIds = getFound(title, currLength, &lenFound); 
					printFound(entryIds, currOffset, lenFound); 
					if (lenFound) highlightOption(0, db[entryIds[0]].title); 
				} 
				break; 
			}
            case 127:
			{
				if (currLength>0) 
				{ 
					moveprint(0, 23+(--currLength), " "); 
					title[currLength] = '\0'; 
					entryIds = getFound(title, currLength, &lenFound); 
					printFound(entryIds, currOffset, lenFound); 
					if (lenFound) highlightOption(0, db[entryIds[0]].title);  
				} 
				break;
			}
            case 189: 
			{ 
				if ((currSelected<MAXENTRIES-1)&&(currSelected<lenFound-1)) 
				{ 
					unhighlightOption(currSelected, db[currSelected+currOffset].title); 
					++currSelected;
					highlightOption(currSelected, db[entryIds[currSelected+currOffset]].title); 
				} 
				else 
				{ 
					if (currOffset<lenFound-MAXENTRIES+((MAXY+1)%4>0)*2) 
					{ 
						++currOffset; 
						printFound(entryIds, currOffset, lenFound); 
						highlightOption(currSelected, db[entryIds[currSelected+currOffset]].title); 
					} 
				} 
				break; 
			} 
            case 188: 
			{ 
				if (currSelected>0&&lenFound) 
				{ 
					unhighlightOption(currSelected, db[entryIds[currSelected+currOffset]].title); 
					--currSelected;
					highlightOption(currSelected, db[entryIds[currSelected+currOffset]].title); 
				} 
				else 
				{ 
					if (currOffset>0) 
					{ 
						--currOffset; 
						printFound(entryIds, currOffset, lenFound); 
						if (lenFound) highlightOption(currSelected, db[entryIds[currSelected+currOffset]].title); 
					} 
				} 
				break; 
			}
            case 182: 
			{ 
				newOptionEntry(title, currLength); 
				entryIds = getFound(title, currLength, &lenFound); 
				currOffset = 0;
				currSelected = 0;
				printFound(entryIds, currOffset, lenFound); 
				if (lenFound) highlightOption(0, db[entryIds[currSelected+currOffset]].title); 
				break; 
			}
			case 13: case 10: 
			{ 
				if(lenFound) 
				{ 
					browseEntry(entryIds[currSelected+currOffset], title); 
					entryIds = getFound(title, currLength, &lenFound); 
					printFound(entryIds, currOffset, lenFound); 
					highlightOption(currSelected, db[entryIds[currSelected+currOffset]].title); 
				} 
				break; 
			}
			case 183: case 3: 
			{ free(entryIds); free(db); deinit(); return 0; }
            case 4: 
			{ 
				removeOption(entryIds[currSelected+currOffset]); 
				currSelected = 0; 
				currOffset = 0; 
				entryIds = getFound(title, currLength, &lenFound); 
				printFound(entryIds, currOffset, lenFound); 
				if (lenFound) highlightOption(0, db[entryIds[0]].title); 
				break; 
			}
            case 5: 
			{ 
				editEntry(entryIds[currSelected+currOffset], currLength); 
				currSelected = 0; 
				currOffset = 0; 
				entryIds = getFound(title, currLength, &lenFound); 
				printFound(entryIds, currOffset, lenFound); 
				if (lenFound) highlightOption(0, db[entryIds[0]].title); 
				break; 
			}
            default:  break;
        }
        move(0, 23+currLength);
    }
    return 0;
}
