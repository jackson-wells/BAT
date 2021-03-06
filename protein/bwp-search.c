#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/syscall.h>
#include "bwp-search.h"

#define HASH_SIZE 101
static struct indexToBase *indexArray[HASH_SIZE];
static struct baseToIndex *baseArray[HASH_SIZE];

float SCORE_CUTOFF = 0.9;
bool EVALUE_BASED_THRESHOLD = true; 
double expectThresh = 1E-8;
int subMatType = 0;
int searchType = 0;
bool showAll = false;
bool silent = false;
bool outputFile = false;
bool verbose = false;
bool filter = true;
int MAX_MISMATCHES = 0;
int MAX_LINE_LENGTH = 100000000;
char INTERVAL_FILE[] = "index.bwp";
char OUTPUT_FILE[100];
int GAP = 11;
int EXTENSION = 1;
int subMat[20][20] = {{4,0,-2,-1,-2,0,-2,-1,-1,-1,-1,-2,-1,-1,-1,1,0,0,-3,-2},{0,9,-3,-4,-2,-3,-3,-1,-3,-1,-1,-3,-3,-3,-3,-1,-1,-1,-2,-2},{-2,-3,6,2,-3,-1,-1,-3,-1,-4,-3,1,-1,0,-2,0,-1,-3,-4,-3},{-1,-4,2,5,-3,-2,0,-3,1,-3,-2,0,-1,2,0,0,-1,-2,-3,-2},{-2,-2,-3,-3,6,-3,-1,0,-3,0,0,-3,-4,-3,-3,-2,-2,-1,1,3},{0,-3,-1,-2,-3,6,-2,-4,-2,-4,-3,0,-2,-2,-2,0,-2,-3,-2,-3},{-2,-3,-1,0,-1,-2,8,-3,-1,-3,-2,1,-2,0,0,-1,-2,-3,-2,2},{-1,-1,-3,-3,0,-4,-3,4,-3,2,1,-3,-3,-3,-3,-2,-1,3,-3,-1},{-1,-3,-1,1,-3,-2,-1,-3,5,-2,-1,0,-1,1,2,0,-1,-2,-3,-2},{-1,-1,-4,-3,0,-4,-3,2,-2,4,2,-3,-3,-2,-2,-2,-1,1,-2,-1},{-1,-1,-3,-2,0,-3,-2,1,-1,2,5,-2,-2,0,-1,-1,-1,1,-1,-1},{-2,-3,1,0,-3,0,1,-3,0,-3,-2,6,-2,0,0,1,0,-3,-4,-2},{-1,-3,-1,-1,-4,-2,-2,-3,-1,-3,-2,-2,7,-1,-2,-1,-1,-2,-4,-3},{-1,-3,0,2,-3,-2,0,-3,1,-2,0,0,-1,5,1,0,-1,-2,-2,-1},{-1,-3,-2,0,-3,-2,0,-3,2,-2,-1,0,-2,1,5,-1,-1,-3,-3,-2},{1,-1,0,0,-2,0,-1,-2,0,-2,-1,1,-1,0,-1,4,1,-2,-3,-2},{0,-1,-1,-1,-2,-2,-2,-1,-1,-1,-1,0,-1,-1,-1,1,5,0,-2,-2},{0,-1,-3,-2,-1,-3,-3,3,-2,1,1,-3,-2,-2,-3,-2,0,4,-3,-1},{-3,-2,-4,-3,1,-2,-2,-3,-3,-2,-1,-4,-4,-2,-3,-3,-2,-3,11,2},{-2,-2,-3,-2,3,-3,2,-1,-2,-1,-1,-2,-3,-1,-2,-2,-2,-1,2,7}};

/* 					*
 * 	MEMORY ALLOCATION FUNCTIONS 	*
 * 					*/



/*
	initializeInputStruct
	
	allocates memory for the variables contained in the input
	structure object

	seqCount: integer variable containing the number of seqeunces presint
                in the input file

        seqLength: an array of integers containing the lengths of each sequence
                in the input file

>	returns: a structure variable prepared to contain information from the
		input file
 
*/
struct input 
initializeInputStruct(int seqCount, int *seqLength)
{
        struct input query;
        int i;
        query.length = (int *) malloc(seqCount * sizeof(int));
        query.name = (char **) malloc(seqCount * sizeof(char *));
        query.sequence = (char **) malloc(seqCount * sizeof(char *));
        for(i = 0; i < seqCount;i++)
        {
                query.name[i] = (char *) malloc(250*(sizeof(char)));
                query.sequence[i] = (char *) malloc(seqLength[i] * sizeof(char));
        }
        return query;
}

/* 				*
 * 	HELPER FUNCTIONS 	*
 * 				*/

int **
calculateExpect(struct FMidx *index,int isc, struct input query,int qsc)
{
	double lambda = 0.271046;
	double K = 0.0471811;
	int i,j;
	int **sThresh = (int **) malloc(qsc * sizeof(int *));
	for(i = 0; i < qsc; i++) 
	{
		sThresh[i] = (int *) malloc(isc * sizeof(int));
		for(j = 0; j < isc; j++)
			sThresh[i][j] = roundFloat((-1/lambda)*log(expectThresh/(K*index[j].length*query.length[i])));
	}
	return sThresh;
}

int 
min(int a, int b)
{
	if(a <= b)
		return a;
	else
		return b;
}

void 
readSubMat(int selection)
{
	int i,j,temp;
	FILE *file;
	if(selection == 1)
	{
		printf("Using blosum90\n");
		system("python ../scripts/printSubsMat.py blosum90");
		file = fopen("blosum90.txt","r");
		for(i = 0; i < 20; i++)
		{
			for(j = 0; j < 20; j++)
			{
				fscanf(file,"\t%d",&temp);
				subMat[i][j] = temp;
			}
		}
		fclose(file);
		system("rm -rf blosum90.txt");
	}
	else if(selection == 2)
	{
		printf("Using pam30\n");
		system("python ../scripts/printSubsMat.py pam30");
                file = fopen("pam30.txt","r");
                for(i = 0; i < 20; i++)
                {
                        for(j = 0; j < 20; j++)
                        {
                                fscanf(file,"\t%d",&temp);
                                subMat[i][j] = temp;
                        }
                }
                fclose(file);
                system("rm -rf pam30.txt");
	}
	else if(selection == 3)
        {
		printf("Using pam60\n");
                system("python ../scripts/printSubsMat.py pam60");
                file = fopen("pam60.txt","r");
                for(i = 0; i < 20; i++)
                {
                        for(j = 0; j < 20; j++)
                        {
                                fscanf(file,"\t%d",&temp);
                                subMat[i][j] = temp;
                        }
                }
                fclose(file);
                system("rm -rf pam60.txt");
        }
	else if(selection == 4)
        {
		printf("Using pam250\n");
                system("python ../scripts/printSubsMat.py pam250");
                file = fopen("pam250.txt","r");
                for(i = 0; i < 20; i++)
                {
                        for(j = 0; j < 20; j++)
                        {
                                fscanf(file,"\t%d",&temp);
                                subMat[i][j] = temp;
                        }
                }
                fclose(file);
                system("rm -rf pam250.txt");
        }
	else
		printf("Invalid substitution matrix selected!\nDefaulting to blosum62\n");
}

int 
roundFloat(float num)
{
    	return num < 0 ? num - 0.5 : num + 0.5;
}


/*  
	seqCount

      	computes the number of sequences in the query file

	fileName: string variable containing the name of the input file	

        returns: an integer variable containing the number of 
		sequence in the input file

*/
int 
getSeqCount(char *fileName) 
{
        FILE *file = fopen(fileName,"r");
        int lineCount = 0;
        char *temp = (char *) malloc((MAX_LINE_LENGTH+1) * sizeof(char));
        while(fgets(temp,MAX_LINE_LENGTH,file) != NULL)
        {
		if(strstr(temp,">"))
                	lineCount++;
        }
        fclose(file);
        free(temp);
        return lineCount;
}


/* 
	removePrefix

 	removes the first two characters from a string

	input: character array containing a sequence with a prefix

 	returns: character array without its first two characters

*/
char *
removePrefix(char *input)
{
	memmove(input, input+2, strlen(input));
	return input;
}


/*  
	getSeqLength

      	get the lengths of query sequences from input file

	fileName: string variable containing the name of the input file

	seqCount: integer variable containing the number of query sequences
              
	returns: array of integers

*/
int *
getSeqLength(char *fileName,int seqCount) /*returns an array of sequence lengths*/
{
	int i = 0;
        int count = 0;
        FILE *file = fopen(fileName,"r");
        char *temp = (char *) malloc(MAX_LINE_LENGTH * sizeof(char));
        int *seqLength = (int *) malloc(seqCount * sizeof(int));
        while(fgets(temp,MAX_LINE_LENGTH-1,file) != NULL)
        {
                if(temp[0] == '>')
		{
                        if(count)
                                i++;    
		}
                else if(temp[0] == '\n')
                        continue;
                else
                {
                        count++;
                        temp[strcspn(temp, "\n")] = 0;
                        temp[strcspn(temp, "\r")] = 0;
                        seqLength[i] += strlen(temp);
                }
        }
        fclose(file);
        /*free(temp);*/
        return seqLength;
}


/*
 
	getLength

	retrieves the lengths of the index sequences

	seqCount: integer varaible containing the number of index sequences

	returns: array of integers

*/
int *
getLength(int seqCount)
{
        FILE *file = fopen(INTERVAL_FILE,"r");
        int *length = (int *) malloc(seqCount*sizeof(int));
        char *temp = (char *) malloc((MAX_LINE_LENGTH+1)*sizeof(char));
        int i = 0;
        while(fgets(temp,MAX_LINE_LENGTH,file) != NULL)
        {
                if(temp[0] == 'l')
                {
                        removePrefix(temp);
                        length[i] = atoi(temp);
                        i++;
                }
        }
        fclose(file);
	free(temp);
        return length;
}

/*  
	readFasta

      	stores FASTA file information into memory

	fileName: string variable containing the name of the input file

	query: array of structures to contain input sequence information
	
*/


void 
readFasta(char *fileName, struct input *query)
{
	char *temp = (char *) malloc(MAX_LINE_LENGTH * sizeof(char));
        char *temp2 = (char *) malloc(MAX_LINE_LENGTH * sizeof(char)); 
        int i = 0; 
	int seqCount= 0;
        FILE *file = fopen(fileName,"r");
        while(fgets(temp,MAX_LINE_LENGTH,file) != NULL)   /*loops through each line of a file*/
        {
                if(temp[0] == '>')      /*if line is a header*/
                {
                        if(seqCount == 0)
                        {
                                strtok(temp,"\n");
                                memmove(temp, temp+1, strlen(temp));
                                strcpy(query->name[i],temp);
                        }
                        else
                        {
				query->length[i] = strlen(temp2);
                                strcpy(query->sequence[i],temp2);/*,query->length[i]);*/
                                i++;
                                memset(temp2,0,strlen(temp2));
                                strtok(temp,"\n");
                                memmove(temp, temp+1, strlen(temp));
                                strcpy(query->name[i],temp);
                        }
                }
                else if(temp[0] == '\n') /*if line is empty*/
                        continue;
                else    /*if line contains an amino acid sequence*/
                {
                        seqCount++;
                        strtok(temp,"\n"); /*strings read from file have extra \n added by file read*/
                        strcat(temp2,temp);
                        memset(temp,0,strlen(temp));
                }
        }
	query->length[i] = strlen(temp2);
        strcpy(query->sequence[i],temp2);/*,query->length[i]);*/
        fflush(file);
        fclose(file);
/*      free(temp);
        free(temp2);*/  
}


/*
 
	reverse

	computes the reverse of a supplied string

	str: character array to be reversed

	returns: a reversed version of the input string

*/
char *
reverse(char *str)
{
	int i,len,end;
	char *tStr;
	len = strlen(str);
	end = len-1;
	tStr = (char *) malloc(sizeof(char) * len);
	for( i=0 ; i< len ; i++)
	{
		tStr[i] = str[end];
		end--;
	}
	return tStr;
}

int 
indexHashCode(int key) 
{
   	return key % HASH_SIZE;
}

void 
createIndexHash(void)
{
	insertIndex(0,'A');
	insertIndex(1,'C');
	insertIndex(2,'D');
	insertIndex(3,'E');
	insertIndex(4,'F');
	insertIndex(5,'G');
	insertIndex(6,'H');
	insertIndex(7,'I');
	insertIndex(8,'K');
	insertIndex(9,'L');
	insertIndex(10,'M');
        insertIndex(11,'N');
        insertIndex(12,'P');
        insertIndex(13,'Q');
        insertIndex(14,'R');
        insertIndex(15,'S');
        insertIndex(16,'T');
        insertIndex(17,'V');
        insertIndex(18,'W');
        insertIndex(19,'Y');
	return;
}

char 
indexSearch(int key)
{
        int hashIndex = indexHashCode(key);
	if(indexArray[hashIndex]->key == key)
        	return indexArray[hashIndex]->data;	
	else
	{
		printf("Invalid key encountered: '%d'\n",key);
        	exit(1);
	}
}

void 
insertIndex(int key,char data) 
{
   	struct indexToBase *item = (struct indexToBase *) malloc(sizeof(struct indexToBase));
	int hashIndex = indexHashCode(key);
   	item->data = data;  
   	item->key = key;
	while(indexArray[hashIndex] != NULL && indexArray[hashIndex]->key != -1) 
	{
		++hashIndex;
		hashIndex %= HASH_SIZE;
	}
	indexArray[hashIndex] = item;
}

unsigned 
baseHashCode(char key)
{
    	return (key + 31) % HASH_SIZE;
}

int 
baseSearch(char key)
{
        if(key == baseArray[baseHashCode(key)]->key)
          	return baseArray[baseHashCode(key)]->data; /* found */
	else
	{
		printf("Invalid key encountered: '%c'\n",key);
        	exit(1);
	}
}

void 
insertBase(char key, int data)
{
    	struct baseToIndex *item;
    	unsigned hashval;
        item = (struct baseToIndex *) malloc(sizeof(*item));
        hashval = baseHashCode(key);
	item->data = data;
	item->key = key;
        baseArray[hashval] = item;
}

void 
createBaseHash(void)
{
        insertBase('A',0);
        insertBase('C',1);
        insertBase('D',2);
        insertBase('E',3);
        insertBase('F',4);
        insertBase('G',5);
        insertBase('H',6);
        insertBase('I',7);
        insertBase('K',8);
        insertBase('L',9);
        insertBase('M',10);
        insertBase('N',11);
        insertBase('P',12);
        insertBase('Q',13);
        insertBase('R',14);
        insertBase('S',15);
        insertBase('T',16);
        insertBase('V',17);
        insertBase('W',18);
        insertBase('Y',19);
        return;
}

/*
 
	fileExists
	
	verify that a supplied file exists and is openable

	temp: character array containing the name of a file

	retunrs:
		0: if the file does not exists
	
		1: if the file does exist

*/
int 
fileExists(char *temp)
{
	
	FILE *file = fopen(temp, "r");
	if (file)
	{
		fclose(file);
		return 1;
	}
	else
		return 0;
}

void 
push(struct matches** head_ref, int k,int l,int score,char *traceback,int traceLength, int keep)
{
        struct matches *new_node = (struct matches *) malloc(sizeof(struct matches));
	new_node->keep = keep;
        new_node->low = k;
        new_node->high = l;
        new_node->score = score;
        new_node->tb = (char *) malloc(sizeof(char) * strlen(traceback));
        strcpy(new_node->tb,traceback);
        new_node->traceLength = traceLength;
        new_node->next = (*head_ref);
        (*head_ref) = new_node;
}

int 
isPresent(struct matches *head, int k, int l)
{
    	struct matches *t = head;
    	while (t != NULL)
    	{
        	if((t->low == k)&&(t->high == l))
                	return 0; /*should be 1 */
        	t = t->next;
    	}
    	return 0;
}

struct matches *
pointToTail(struct matches *match)
{
        if(match != NULL)
        {
                while(match->next != NULL)
                        match = match->next;
        }
        return match;
}

void 
frontbacksplit(struct matches* source, struct matches** frontRef, struct matches** backRef)
{
    	struct matches* fast;
    	struct matches* slow;
    	if(source == NULL || source->next == NULL)
    	{
        	*frontRef = source;
       		*backRef = NULL;
    	}
    	else
    	{
        	slow = source;
        	fast = source->next;
        	while(fast != NULL)
        	{
            		fast = fast->next;
            		if(fast != NULL)
            		{
                		slow = slow->next;
                		fast = fast->next;
            		}
        	}
    	}
    	*frontRef = source;
    	*backRef = slow->next;
    	slow->next = NULL;
}

/*
 
	getCount

	gets the number of sequences contained in the index file
	
	returns: an integer variable corresponding to the number
		of sequences inside the index file

*/
int 
getCount(void)
{
	char temp[10];
	FILE *file = fopen(INTERVAL_FILE,"r");
	fgets(temp,10,file);
	fclose(file);
	removePrefix(temp);
	return atoi(temp);
}

struct matches *
getUnion(struct matches *head1, struct matches *head2)
{
        struct matches *result = NULL;
        struct matches *t1 = head1, *t2 = head2;
        while (t1 != NULL)
        {
                push(&result, t1->low,t1->high,t1->score,t1->tb,t1->traceLength, t1->keep); /*put new matches at top of stack*/
                t1 = t1->next;
        }
        while (t2 != NULL)
        {
/*              if(!isPresent(result, t2->low,t2->high))
 *              {*/
                push(&result, t2->low,t2->high,t2->score,t2->tb,t2->traceLength, t2->keep);
/*              }*/
                t2 = t2->next;
        }
        return result;
}

/*					*
 *	INPUT HANDLING FUNCTIONS	*
 *					*/

/*

      	manageInputs

        handles command-line input from user

        argc: integer variable containing the number of arguements passed in

        argv: 2d character array containing the arguement strings

        seqCount: integer array to hold the number of sequences to be processed
        	in the run of the program

	returns: a structure variable containing input information
                                                                 */
struct input 
manageInputs(char *argv[], int argc, int *seqCount)
{
        struct input query;
        int c;
        if(argc <= 1)   /*no arguements supplied*/
        {
                printf("Please provide the necessary options\n\nuse -h for usage statement\n");
                exit(0);
        }
	opterr = 0;
        while ((c = getopt (argc, argv, "OvShf:s:M:o:d:i:a:m:c:e:")) != -1) /*options must be added here to be recognized, options followed by : take in a parameter*/
        {
                switch (c)
                {
                        case 'h':
                                printf("\nBurrows Wheeler Protein Aligner\n\nUsage: \"bwp-search <options>\"\n\nOptions:\n\n-f\t\tFor input of a fasta file as a query\n-s\t\tFor input of a string as a query\n-h\t\tFor this usage statement\n-S\t\tTo suppress all output to screen\n-M\t\tTo designate maximum sequence length according to character count\n-o\t\tSpecify output file name (exclude file extentions)\n");
				printf ("-e\t\tTo designate Evalue cutoff e.g. -e 1E-10\n-c\t\tTo designate a score based cutoff e.g. -c 0.9 (scored searches only)\n-d\t\tTo designate the number of allowed mismatches\n-i\t\tTo specify a custom index file\n-v\t\tTo supply output to screen\n-a [1,2]\tTo designate search algorithm (default: scored)\n   1\tDistance\n   2\tConserved Distance\n");
				printf("-O\t\tTo display matches not meeting the score threshold\n-m [1,2,3,4]\tTo designate scoring matrix (default: blosum62)\n   1\tblosum90\n   2\tpam30\n   3\tpam60\n   4\tpam250\n\n");
                                exit(0);

                        case 'f':
                                if(fileExists(optarg))
                                {
                                        handleF(&query,optarg);
                                        *seqCount = getSeqCount(optarg);
                                        break;
                                }
                                else
                                {
                                        printf("%s not found, exiting\n",optarg);
                                        exit(0);
                                }

                        case 's' :
                                handleS(&query,optarg);
                                *seqCount = 1;
                                break;
			case 'a' :
				searchType = atoi(optarg);
				break;
			case 'm' :
				subMatType = atoi(optarg);
				break;

                        case '?' :
                                if(optopt == 'f' 
				   || optopt == 's' 
				   || optopt == 'M' 
				   || optopt == 'o' 
				   || optopt == 'd' 
				   || optopt == 'i' 
				   || optopt == 'a' 
				   || optopt == 'm' 
				   || optopt == 'c' 
				   || optopt == 'e')
                                    fprintf (stderr, "Option -%c requires an argument.\n\nuse -h for usage statement\n", optopt);
				else if(isprint (optopt))
                                    fprintf (stderr, "Unknown option `-%c'.\n\nuse -h for usage statement\n", optopt);
                                exit(0);
                        case 'M' :
                                MAX_LINE_LENGTH = atoi(optarg);
                                break;
                        case 'o' :
				outputFile = true;
                                strcpy(OUTPUT_FILE,optarg);
                                break;
                        case 'v' :
                                verbose = true;
				filter = false;
                                break;
                        case 'd' :
                                MAX_MISMATCHES = atoi(optarg);
                                break;
                        case 'i' :
                                if(fileExists(optarg))
                                        strcpy(INTERVAL_FILE,optarg);
                                else
                                {
                                        printf("%s is not a valid file\nExiting\n",optarg);
                                        exit(0);
                                }
                                break;
			case 'c' :
				EVALUE_BASED_THRESHOLD = false;
				SCORE_CUTOFF = atof(optarg);
				break;
			case 'e' :
				expectThresh = atof(optarg);
				break; 
			case 'O' :
				showAll = true;
				break;
                        case 'S' :
                                silent = true;
                                break;
                }
        }
        if(optind < argc)
        {
                printf ("Non-option arguments supplied\n\nuse -h for usage statement\n");
                exit(0);
        }
        return query;
}

/*

        handleS

       	handles sequence information supplied via command-line

       	query: structure variable containing input information

	sequence: protein sequence input by the user

*/
void 
handleS(struct input *query, char *sequence)
{
        int seqC = 1;
        int *seqL = (int *) malloc(sizeof(int));
        seqL[0] = strlen(sequence);
        *query = initializeInputStruct(seqC,seqL);
        query->length[0] = seqL[0];
        strcpy(query->sequence[0],sequence);
        strcpy(query->name[0],"String Input");
}


/*

       	handleF

     	handles sequence information supplied via files

        query: structure variable containing file information

	fileName: character string containing the name of the input file

*/
void 
handleF(struct input *query,char *fileName)
{
        int seqCount = getSeqCount(fileName);
        int *seqLength = getSeqLength(fileName,seqCount);
        *query = initializeInputStruct(seqCount,seqLength);
        readFasta(fileName, query);
}

/*
 
	getIndex



	IseqCount:

	returns:

*/
struct FMidx *
getIndex(void)
{
	int j,z,i,rCount,oCount;
	char *number;
	char *temp;
	FILE *file;
	struct FMidx *tempIndex;
	int seqCount = getCount();
	int *seqLength = getLength(seqCount);
	temp = (char *) malloc((MAX_LINE_LENGTH+1)*sizeof(char));
	tempIndex = (struct FMidx *) malloc(seqCount * sizeof(struct FMidx));
	file = fopen(INTERVAL_FILE,"r");
	for(i = 0; i < seqCount; i++)
	{
		tempIndex[i].O = (int **) malloc(20 * sizeof(int *));
		tempIndex[i].R = (int **) malloc(20 * sizeof(int *));
		for(j = 0; j < 20; j++)
		{
			tempIndex[i].R[j] = (int *) malloc(seqLength[i] * sizeof(int));
			tempIndex[i].O[j] = (int *) malloc(seqLength[i] * sizeof(int));
		}
		tempIndex[i].length = seqLength[i];
	}
	i = 0;
	oCount = 0;
	rCount = 0;
    	while(fgets(temp,MAX_LINE_LENGTH,file) != NULL)   /*loops through each line of a file*/
    	{
		strtok(temp,"\n");
		if(temp[0] == '\n')
			continue;
		else if(temp[0] == 'n')
			continue;
		else if(temp[0] == 'd')
		{
			removePrefix(temp);
			tempIndex[i].desc = (char *) malloc(250*sizeof(char));
			strcpy(tempIndex[i].desc,temp);
		}
		else if(temp[0] == 'l')
			continue;
		else if(temp[0] == 'q')
		{
			removePrefix(temp);
			tempIndex[i].sequence = (char *) malloc(tempIndex[i].length*sizeof(char));
			strncpy(tempIndex[i].sequence,temp,tempIndex[i].length);
		}
		else if(temp[0] == 's')
                {
                        removePrefix(temp);
			tempIndex[i].SA = (int *) malloc(tempIndex[i].length*sizeof(int));
			number = strtok(temp," ");
			tempIndex[i].SA[0] = atoi(number);
			for(j = 1; j < tempIndex[i].length; j++)
			{
				number = strtok(NULL," ");
				tempIndex[i].SA[j] = atoi(number);
			}
                }
		else if(temp[0] == 't')
                {
                        removePrefix(temp);
			tempIndex[i].transform = (char *) malloc(tempIndex[i].length*sizeof(char));
			strncpy(tempIndex[i].transform,temp,tempIndex[i].length);
                }
		else if(temp[0] == 'f')
                {
                        removePrefix(temp);
                        tempIndex[i].reverse = (char *) malloc(tempIndex[i].length*sizeof(char));
                        strncpy(tempIndex[i].reverse,temp,tempIndex[i].length);
                }
		else if(temp[0] == 'c')
                {
                        removePrefix(temp);
                        number = strtok(temp," ");
                        tempIndex[i].C[0] = 0;
                        for(j = 1; j < 20; j++)
                        {
                                number = strtok(NULL," ");
                                tempIndex[i].C[j] = atoi(number);
                        }
                }
		else if(temp[0] == 'o')
                {
			removePrefix(temp);
			number = strtok(temp," ");
			tempIndex[i].O[oCount][0] = atoi(number);
			for(z = 1; z < tempIndex[i].length; z++)
			{
                              	number = strtok(NULL," ");
                               	tempIndex[i].O[oCount][z] = atoi(number);
			}
			oCount++;
			if(oCount >= 20)
				oCount = 0;
                }
		else if(temp[0] == 'r')
                {
                        removePrefix(temp);
                        number = strtok(temp," ");
                        tempIndex[i].R[rCount][0] = atoi(number);
                        for(z = 1; z < tempIndex[i].length; z++)
                        {
                                number = strtok(NULL," ");
                                tempIndex[i].R[rCount][z] = atoi(number);
                        }
                        rCount++;
                        if(rCount >= 20)
                        {
                                rCount = 0;
                                i++;
                        }
                }
		memset(temp,0,strlen(temp));
	}
	fclose(file);
	return tempIndex;
}

/*				*
 *	SORTING FUCNTIONS	*
 *				*/

struct matches *
sortMatches(struct matches *match, struct FMidx input)
{
        mergeSortMatches(&match); /*returns linked listed sorted by score*/
	filterMatches(&match,input);
        return match;
}

void 
filterMatches(struct matches **headRef, struct FMidx input)
{
	int oLength,s1,s2,e1,e2,length;
	float overlap;
	struct matches *node = *headRef;
	if(node != NULL)
	{
		s1 = input.SA[node->low];
                e1 = input.SA[node->high] + node->traceLength;
	}
	while(node != NULL && node->next != NULL)
	{
		s2 = input.SA[node->next->low];
		e2 = input.SA[node->next->high] + node->next->traceLength;
		length = min((e1-s1+1),(e2-s2+1));
		if(s1 <= s2 && s2 <= e1)
		{
			if(e1 < e2)
				oLength = e1 - s2 + 1;
			else
				oLength = e2 - s2 + 1;
		}
		else if(s1 <= e2 && e2 <= e1)
		{
			if(s2 < s1)
                               	oLength = e2 - s1 + 1;
                       	else
                               	oLength = e2 - s2 + 1;
		}
		else if(s2 <= s1 && s1 <= e2)
		{
			if(e2 < e1)
                               	oLength = e2 - s1 + 1;
                       	else
                               	oLength = e1 - s1 + 1;
		}
		overlap = (float) oLength/length;
		if(overlap > 0.75)
			node->next->keep = 0;
		node = node->next;
	}
	return;
}

void 
mergeSortMatches(struct matches** headRef)
{
    	struct matches* head = *headRef;
    	struct matches* a;
    	struct matches* b;
    	if ((head == NULL) || (head -> next == NULL))
        	return;
    	frontbacksplit(head, &a, &b);
    	mergeSortMatches(&a);
    	mergeSortMatches(&b);
    	*headRef = sortedmergeMatch(a, b);
}

struct matches* 
sortedmergeMatch(struct matches* a, struct matches* b)
{
    	struct matches* result = NULL;
    	if (a == NULL)
        	return(b);
    	else if (b == NULL)
        	return(a);
    	if(a->score >= b->score)
    	{
        	result = a;
        	result->next = sortedmergeMatch(a->next, b);
    	}
    	else
    	{
    	    	result = b;
    	    	result->next = sortedmergeMatch(a, b->next);
    	}
    	return(result);
}

/*				*
 *	SEARCH FUNCTIONS	*
 *				*/

int *
calculateT(int qsc, struct input query) 
{
	int i,j;
	int *tempThresh = (int *) malloc(qsc*sizeof(int));
	for(i = 0; i < qsc; i++) {
		tempThresh[i] = 0;
		for(j = 0; j < query.length[i]; j++)
			 tempThresh[i] += subMat[baseSearch(query.sequence[i][j])][baseSearch(query.sequence[i][j])];
		tempThresh[i] = roundFloat(SCORE_CUTOFF * tempThresh[i]);
	}
	return tempThresh;
}

int ***
calculateS(struct FMidx *index,int isc, struct input query,int qsc)
{
        int i,j,z,k,l,high,low,s;
        int ***score = (int ***) malloc(qsc*sizeof(int **));
        for(i = 0; i < qsc; i++)
        {
                score[i] = (int **) malloc(isc*sizeof(int *));
                for(z = 0; z < isc; z++)
                {
                        score[i][z] = (int *) calloc(query.length[i],sizeof(int));
                        low = 1;
                        high = index[z].length - 1;
                        s = 0;
                        for(j = 0; j < query.length[i]; j++)
                        {
                                k = index[z].C[baseSearch(query.sequence[i][j])] + index[z].R[baseSearch(query.sequence[i][j])][low-1] + 1;
                                l = index[z].C[baseSearch(query.sequence[i][j])] + index[z].R[baseSearch(query.sequence[i][j])][high];
                                if(index[z].reverse[0] == query.sequence[i][j])
                                        k = k - 1;
                                if(k <= l)
                                {
					if(j < 1) 
						s = score[i][z][0] + subMat[baseSearch(query.sequence[i][j])][baseSearch(query.sequence[i][j])];
					else
						s = score[i][z][j-1] + subMat[baseSearch(query.sequence[i][j])][baseSearch(query.sequence[i][j])];
				}
				else
				{
/*					if(i == 0 && j == 0) {
						printf("ADDINGG GAP PENALTY TO EXACT MATCH\n");
					}*/
					low = 1;
                                        high = index[z].length - 1;
					if(j < 1)
						s = score[i][z][0] - GAP;
					else
                                        	s = score[i][z][j-1] - GAP;
                                }
                                score[i][z][j] = s;
                        }
                }
        }
        return score;
}

int ***
calculateD(struct FMidx *index,int isc, struct input query,int qsc)
{
	int i,j,z;
	int ***distance = (int ***) malloc(qsc*sizeof(int **));
	int low = 1;
	int d = 0;
	int high;
	int k,l;
	for(i = 0; i < qsc; i++)
	{
		distance[i] = (int **) malloc(isc*sizeof(int *));
		for(z = 0; z < isc; z++)
		{
			distance[i][z] = (int *) malloc(query.length[i]*sizeof(int));
			low = 1;
			high = index[z].length - 1;
			d = 0;
			for(j = 0; j < query.length[i]; j++)
			{
				k = index[z].C[baseSearch(query.sequence[i][j])] + index[z].R[baseSearch(query.sequence[i][j])][low-1] + 1;
				l = index[z].C[baseSearch(query.sequence[i][j])] + index[z].R[baseSearch(query.sequence[i][j])][high];
				if(index[z].reverse[0] == query.sequence[i][j])
					k = k - 1;
				if(k > l)
				{
					low = 1;
					high = index[z].length - 1;
					d = d + 1;
				}
				distance[i][z][j] = d;
			}
		}
		
	}
	return distance;
}

struct output **
exactSearch(struct input query,int qsc,struct FMidx *index,int isc)
{
        int z,j,i,low,high;
        char c;
        struct output **temp = (struct output **) malloc(qsc*sizeof(struct output *));
        for(z = 0; z < qsc; z++)
        {
                temp[z] = (struct output *) malloc(isc*sizeof(struct output));
                for(j = 0; j < isc; j++)
                {
                        temp[z][j].sequence = (char *) malloc(query.length[z]*sizeof(char));
                        strcpy(temp[z][j].sequence,query.sequence[z]);
                        i = query.length[z]-1;
                        c = query.sequence[z][i];
                        low = index[j].C[baseSearch(c)] + 1;
                        high = index[j].C[baseSearch(c)+1];
                        while(low <= high && 1 <= i)
                        {
                                c = query.sequence[z][i-1];
                                low = index[j].C[baseSearch(c)] + index[j].O[baseSearch(c)][low-2] +1;
                                high = index[j].C[baseSearch(c)] + index[j].O[baseSearch(c)][high-1]; /*-1 for 0-base*/
                                i = i - 1;
                        }
                        if(high < low)
                        {
                                temp[z][j].low = 0;
                                temp[z][j].high = 0;
                        }
                        else
                        {
                                temp[z][j].low = low-1;
                                temp[z][j].high = high-1;/*index[j].SA[high-1];*/
                        }
                }
        }
        return temp;
}

struct results **
distanceSearch(struct input query,int qsc,struct FMidx *index,int isc,int ***D)
{
        int i,j;
        struct results **temp = (struct results **) malloc(qsc*sizeof(struct results *));
        char *traceBack;
        for(i = 0; i < qsc; i++)
        {
                temp[i] = (struct results *) malloc(isc*sizeof(struct results));
                for(j = 0; j < isc; j++)
                {
                        temp[i][j].match = NULL;
                        traceBack = (char *) malloc(sizeof(char) * (MAX_MISMATCHES+query.length[i]));
                        temp[i][j].match = distanceRecur(index[j],D[i][j],query.sequence[i],query.length[i]-1,MAX_MISMATCHES,1,index[j].length-1,0,1,traceBack,0);
                        temp[i][j].match = sortMatches(temp[i][j].match,index[j]);
                }
        }
        return temp;
}

struct matches *
distanceRecur(struct FMidx index, int *D,char *W,int i,int d, int low, int high,
	      int score,int pState,char *traceBack,int tbIdx)
{
        int j;
        char *tempTB = (char *) malloc(sizeof(char)*tbIdx);
        int tempP = pState;
        int tempS = score;
        struct matches *match = (struct matches *) malloc(sizeof(struct matches));
        struct matches *results = NULL;
        strcpy(tempTB,traceBack);
        match->next = NULL;
        if(i < 0)
	{
                if(d < D[0])
			return NULL;
                match->low = low;
                match->high = high;
                match->score = score;
                match->tb = (char *) malloc(sizeof(char) * tbIdx);
                strncpy(match->tb,tempTB,tbIdx);
                strcpy(match->tb,reverse(match->tb));
                match->traceLength = strlen(match->tb);
                match->keep = 1;
                return match;
        }
	else
	{
                if(d < D[i])
                        return NULL;
	}
        tempTB[tbIdx] = 'X';
        match = distanceRecur(index,D,W,i-1,d-1,low,high,getScore(0,0,tempS,tempP,2),2,tempTB,tbIdx+1); /*GAP in index seq*/
        if(match != NULL)
		results = getUnion(match,results);
        match = NULL;
        tempP = pState;
        tempS = score;
        for(j = 0; j < 20; j++)
        {
                int k = index.C[j] + index.O[j][low-1] + 1;
                int l = index.C[j] + index.O[j][high];
                if(index.transform[0] == indexSearch(j) && low == 1 && (high == index.length-1))
                        k = k - 1;
                if(k <= l)
                {
                        tempTB[tbIdx] = 'Y';
                        match = distanceRecur(index,D,W,i,d-1,k,l,getScore(j,baseSearch(W[i]),tempS,tempP,3),3,tempTB,tbIdx+1); /*GAP in query*/
                        if(match != NULL)
				results = getUnion(match,results);
                        match = NULL;
                        tempP = pState;
                        tempS = score;
                        if(indexSearch(j) == W[i]) /*match*/
                        {
                                tempTB[tbIdx] = 'M';
                                match = distanceRecur(index,D,W,i-1,d,k,l,getScore(j,baseSearch(W[i]),tempS,tempP,1),1,tempTB,tbIdx+1);
                        }
                        else /*mismatch*/
                        {
                                tempTB[tbIdx] = 'U';
                                match = distanceRecur(index,D,W,i-1,d-1,k,l,getScore(j,baseSearch(W[i]),tempS,tempP,1),1,tempTB,tbIdx+1);
			}
                       	if(match != NULL)
				results = getUnion(match,results);
                       	match = NULL;
                        tempP = pState;
                      	tempS = score;
                }
        }
        return results;
}

struct results **
conservedSearch(struct input query,int qsc,struct FMidx *index,int isc,int ***D)
{
	int i,j;
	struct results **temp = (struct results **) malloc(qsc*sizeof(struct results *));
	char *traceBack;
	for(i = 0; i < qsc; i++)
	{
		temp[i] = (struct results *) malloc(isc*sizeof(struct results));
		for(j = 0; j < isc; j++)
		{
			temp[i][j].match = NULL;
			traceBack = (char *) malloc(sizeof(char) * (MAX_MISMATCHES+query.length[i]));
			temp[i][j].match = conservedRecur(index[j],D[i][j],query.sequence[i],query.length[i]-1,MAX_MISMATCHES,1,index[j].length-1,0,1,traceBack,0);
			temp[i][j].match = sortMatches(temp[i][j].match,index[j]);
		}
	}
	return temp;
}

struct matches *
conservedRecur(struct FMidx index, int *D,char *W,int i,int d, int low,
               int high,int score,int pState,char *traceBack,int tbIdx)
{
        int j;
        char *tempTB = (char *) malloc(sizeof(char)*tbIdx);
        int tempP = pState;
        int tempS = score;
        struct matches *match = (struct matches *) malloc(sizeof(struct matches));
        struct matches *results = NULL;
        strcpy(tempTB,traceBack);
        match->next = NULL;
        if(i < 0)
        {
                if(d < D[0])
                        return NULL;
                match->low = low;
                match->high = high;
                match->score = score;
                match->tb = (char *) malloc(sizeof(char) * tbIdx);
                strncpy(match->tb,tempTB,tbIdx);
                strcpy(match->tb,reverse(match->tb));
                match->traceLength = strlen(match->tb);
                match->keep = 1;
                return match;
        }
        else
        {
                if(d < D[i])
                        return NULL;
        }
        tempTB[tbIdx] = 'X';
        match = conservedRecur(index,D,W,i-1,d-1,low,high,getScore(0,0,tempS,tempP,2),2,tempTB,tbIdx+1); /*GAP in index seq*/
        if(match != NULL)
                results = getUnion(match,results);
        match = NULL;
        tempP = pState;
        tempS = score;
        for(j = 0; j < 20; j++)
        {
                int k = index.C[j] + index.O[j][low-1] + 1;
                int l = index.C[j] + index.O[j][high];
                if(index.transform[0] == indexSearch(j) && low == 1 && (high == index.length-1))
                        k = k - 1;
                if(k <= l)
                {
                        tempTB[tbIdx] = 'Y';
                        match = conservedRecur(index,D,W,i,d-1,k,l,getScore(j,baseSearch(W[i]),tempS,tempP,3),3,tempTB,tbIdx+1); /*GAP in query*/
                        if(match != NULL)
                                results = getUnion(match,results);
                        match = NULL;
                        tempP = pState;
                        tempS = score;
                        if(indexSearch(j) == W[i]) /*match*/
                        {
                                tempTB[tbIdx] = 'M';
                                match = conservedRecur(index,D,W,i-1,d,k,l,getScore(j,baseSearch(W[i]),tempS,tempP,1),1,tempTB,tbIdx+1);
                        }
                        else /*mismatch*/
                        {
                                tempTB[tbIdx] = 'U';
                                if(subMat[j][baseSearch(W[i])] > 0)
                                        match = conservedRecur(index,D,W,i-1,d,k,l,getScore(j,baseSearch(W[i]),tempS,tempP,1),1,tempTB,tbIdx+1);
                                else
                                        match = conservedRecur(index,D,W,i-1,d-1,k,l,getScore(j,baseSearch(W[i]),tempS,tempP,1),1,tempTB,tbIdx+1);
                        }
                        if(match != NULL)
                                results = getUnion(match,results);
                        match = NULL;
                        tempP = pState;
                        tempS = score;
                }
        }
        return results;
}

struct results **
scoredSearch(struct input query,int qsc,struct FMidx *index,int isc,int ***S)
{
        int i,j;
	int *sThresh90;
	int **sThreshE;
	struct results **temp = (struct results **) malloc(qsc*sizeof(struct results *));
        char *traceBack;
	if(EVALUE_BASED_THRESHOLD)
		sThreshE = calculateExpect(index, isc, query, qsc);
	else
		sThresh90 = calculateT(qsc,query);
        for(i = 0; i < qsc; i++)
        {
                temp[i] = (struct results *) malloc(isc*sizeof(struct results));
                for(j = 0; j < isc; j++)
                {
                        temp[i][j].match = NULL;
                        traceBack = (char *) malloc(sizeof(char) * (query.length[i]));
			if(EVALUE_BASED_THRESHOLD) 
				temp[i][j].match = scoredRecur(index[j],S[i][j],query.sequence[i],query.length[i]-1,1,index[j].length-1,0,1,traceBack,0,sThreshE[i][j]);
			else
                        	temp[i][j].match = scoredRecur(index[j],S[i][j],query.sequence[i],query.length[i]-1,1,index[j].length-1,0,1,traceBack,0,sThresh90[i]);
                        temp[i][j].match = sortMatches(temp[i][j].match,index[j]);
                }
        }
        return temp;
}

struct matches *
scoredRecur(struct FMidx index,int *sPre,char *W,int i,int low, int high,
	    int sPostIn,int pState,char *traceBack,int tbIdx, int sThresh)
{
        int j;
        char *tempTB = (char *) malloc(sizeof(char)*tbIdx);
        int tempP = pState;
        int sPost = sPostIn;
        struct matches *match = (struct matches *) malloc(sizeof(struct matches));
        struct matches *results = NULL;
/*	if(!silent)
	{
		printf("sPre[%d]:%d\nsPostIn:%d\n\n",i,sPre[i],sPostIn);
		printf("tbIDX: %d\ntb: %s\n\n",tbIdx,traceBack);
	}*/
        strcpy(tempTB,traceBack);
        match->next = NULL;
        if(i < 0)
        {
                if(sPost < sThresh)
			return NULL;
		else
		{
			match->low = low;
	                match->high = high;
	                match->score = sPostIn;
	                match->tb = (char *) malloc(sizeof(char) * tbIdx);
	                strncpy(match->tb,tempTB,tbIdx);
	                strcpy(match->tb,reverse(match->tb));
	                match->traceLength = strlen(match->tb);
			match->keep = 1;
			return match;
		}
        }
        else
	{
		if(sPost < 0)
			return NULL;
		if(tbIdx > (int) strlen(W))
			return NULL;
                if((sPre[i] + sPost) < sThresh) 
			return NULL;
	}
        tempTB[tbIdx] = 'X';				/*Gap in index*/
        match = scoredRecur(index,sPre,W,i-1,low,high,getScore(0,0,sPost,tempP,2),2,tempTB,tbIdx+1,sThresh);
        if(match != NULL)
		results = getUnion(match,results);
        match = NULL;
        tempP = pState;
        sPost = sPostIn;
        for(j = 0; j < 20; j++)
        {
                int k = index.C[j] + index.O[j][low-1] + 1;
                int l = index.C[j] + index.O[j][high];
                if(index.transform[0] == indexSearch(j) && low == 1 && (high == index.length-1))
                        k = k - 1;
                if(k <= l)
                {
                        tempTB[tbIdx] = 'Y'; 		/*Gap in query*/
                        match = scoredRecur(index,sPre,W,i,k,l,getScore(j,baseSearch(W[i]),sPost,tempP,3),3,tempTB,tbIdx+1,sThresh);
                        if(match != NULL)
				results = getUnion(match,results);
                        match = NULL;
                        tempP = pState;
                        sPost = sPostIn;
                        if(indexSearch(j) == W[i])
				tempTB[tbIdx] = 'M'; 	/*Match*/
                        else
				tempTB[tbIdx] = 'U'; 	/*Mismatch*/
                        match = scoredRecur(index,sPre,W,i-1,k,l,getScore(j,baseSearch(W[i]),sPost,tempP,1),1,tempTB,tbIdx+1,sThresh);
                        if(match != NULL)
				results = getUnion(match,results);
                        match = NULL;
                        tempP = pState;
                        sPost = sPostIn;
                }
        }
        return results;
}

int 
getScore(int l1, int l2,int score, int p, int c)
{
	int temp = score;
	if(p <= 1)	/*Previous State is match */
	{
		if(c == 1) /*match*/
                {
                        temp = temp + subMat[l1][l2];
                        return temp;
                }
                else if(c == 2) /*GAP in index*/
                {
                        temp = temp - GAP;
                        return temp;
                }
                else if(c == 3) /*GAP in query*/
                {
                        temp = temp - GAP;
			return temp;
                }
		else
                {
                        printf("Invalid current state value '%d'\n",c);
                        exit(0);
                }
	}
	else if(p == 2)	/*Previous State is GAP in index */
	{
		if(c == 1) /*match*/
                {
                        temp = temp + subMat[l1][l2];
                        return temp;
                }
                else if(c == 2) /*extension of GAP in index*/
                {
                        temp = temp - EXTENSION;
                        return temp;
                }
                else if(c == 3) /*GAP in query*/
                {
                        temp = temp - GAP;
                        return temp;
                }
		else
                {
                        printf("Invalid current state value '%d'\n",c);
                        exit(0);
                }
	}
	else if(p == 3)	/*Previous State is GAP in query */
	{
		if(c == 1) /*match*/
                {
                        temp = temp + subMat[l1][l2];
                        return temp;
                }
                else if(c == 2) /*GAP in index*/
                {
                        temp = temp - GAP;
                        return temp;
                }
                else if(c == 3) /*extension of GAP in query*/
                {
                        temp = temp - EXTENSION;
                        return temp;
                }
		else
        	{
        	        printf("Invalid current state value '%d'\n",c);
                	exit(0);
        	}
	}
	else
	{
		printf("Invalid previous state value '%d'\n",p);
		exit(0);
	}
}

/*				*
 *	OUTPUT FUNCTIONS	*
 *				*/

void 
outputToFile(struct results **out, int qsc, int isc,struct FMidx *index,struct input query)
{
        FILE *f;
	int i,j;
	struct matches *temp;
        if(strlen(OUTPUT_FILE) == 0)
                f = fopen("out.bed","w");
        else
        {
                strcat(OUTPUT_FILE,".bed");
                f = fopen(OUTPUT_FILE,"w");
        }
/*      write each instance of M to a file*/
        if(f == NULL)
        {
                printf("Error opening file!\n");
                exit(1);
        }
	fprintf(f,"Database\tStart Pos\tEnd Pos\tQuery\tScore\n----------------------------------------------------------------\n");
        for(i = 0; i < qsc; i++)
        {
                for(j = 0; j < isc; j++)
                {
			temp = (struct matches *) malloc(sizeof(struct matches));
                        temp = out[i][j].match;
                        if(temp != NULL)
                        {
                                while(temp != NULL)
                                {
					if(temp->keep != 0 || showAll)
                                        {
						fprintf(f,"%s\t",index[j].desc);
        	                                fprintf(f,"%d\t%d\t%s\t%d\n",index[j].SA[temp->low],query.length[i]+index[j].SA[temp->low]-1,query.name[i],temp->score);
					}
        	                        temp = temp->next;
                                }
                        }
                }
        }
        fclose(f);
}

/*char *getSequenceAlignment(int i,int j,struct FMidx *index, struct input query,struct matches *match)
{
	int z,q,miss,nLcount,temp;
	char *sequence;
	if(match->traceLength > 60)
	{
		nLcount = (match->traceLength/60) + 3;
		sequence = (char *) malloc((nLcount+match->traceLength)*sizeof(char));
	}
	else
	{
		sequence = (char *) malloc(match->traceLength*sizeof(char));
	}
	temp = 0;
	for(q = 0; q < nLcount; q++)
	{
		miss = 0;
		for(z = 0; z < match->traceLength; z++)
		{
			if(z < 60)
			{
				if(match->tbi[z] == 1 || match->tbi[z] == 3)
				{
					sequence[z] = index[j].sequence[index[j].SA[match->low]+(z-1-miss)];
					printf("%c\n",index[j].sequence[index[j].SA[match->low]+(z-1-miss)]);
				}
				else if(match->tbi[z] == 2)
				{
					sequence[z] = '-';
					miss++;
				}
			}
			else
			{
				sequence[z] = '\n';
				temp = z+1+temp;
				z = z + match->traceLength;
			}
		}
		miss = 0;
		for(z = 0; z < match->traceLength; z++)
                {
			if(z < 60)
                        {
                                if(match->tbi[z] == 1)
                                {
					if(index[j].sequence[index[j].SA[match->low]+(z-1-miss)] == query.sequence[i][z-miss])
					{
						sequence[z+temp] = '|';
					}
					else
					{
						sequence[z+temp] = ' ';
					}
                                }
				else if(match->tbi[z] == 3 || match->tbi[z] == 2)
				{
					sequence[z+temp] = ' ';
				}
                        }
                        else
                        {
                                sequence[z] = '\n';
                                temp = z+1+temp;
                                z = z + match->traceLength;
                        }
		}
		miss = 0;
                for(z = 0; z < match->traceLength; z++)
                {
                        if(z < 60)
                        {
                                if(match->tbi[z] == 1 || match->tbi[z] == 2)
                                {
                                        sequence[z+temp] = query.sequence[i][z-miss];
                                }
                                else if(match->tbi[z] == 3)
                                {
                                        sequence[z] = '-';
                                        miss++;
                                }
                        }
                        else
                        {
                                sequence[z] = '\n';
				sequence[z+1] = '\n';
                                z = z + match->traceLength;
                        }
                }
	}
	return sequence;
}*/

/*				*
 *	PRINTING FUNCTIONS	*
 * 				*/

/*
 
	printInResults

	out:

	qsc:

	isc:

	index:

	query:

*/ 

void 
printInResults(struct results **out,int qsc,int isc,struct FMidx *index, struct input query)
{
	int i,j,z;
	int miss = 0;
	int missX = 0;
        int missY = 0;
	struct matches *temp;
	for(i = 0; i < qsc; i++)
	{
		for(j = 0; j < isc; j++)
		{
			temp = (struct matches *) malloc(sizeof(struct matches));
			temp = out[i][j].match; /*causing seg fault*/
			if(temp != NULL)
			{
				 printf("\nindex: %s\n\nquery: %s\n\n-----------------------------------------------\n\n",index[j].desc,query.name[i]);
				while(temp != NULL)
				{
					if(temp->keep != 0 || showAll)
					{
	/*					printf("tb:%s\n",temp->tb);*/
						printf("Score: %d\n\nIndex\t",temp->score);
			/*			char *sequence = getSequenceAlignment(i,j,index,query,temp);
						printf("%s",sequence);*/
						miss = 0;
						for(z = 0; z < temp->traceLength; z++)
						{
						
							if(z % 59 == 0 && z != 0)
								printf("\n");
							else
							{
								if(temp->tb[z] == 'M' || temp->tb[z] == 'U')
									printf("%c",index[j].sequence[index[j].SA[temp->low]+(z-1-miss)]);
								else if(temp->tb[z] == 'X')
								{
									printf("-");
									miss++;
								}
								else if(temp->tb[z] == 'Y')
									printf("%c",index[j].sequence[index[j].SA[temp->low]+(z-1-miss)]);
							}
						}
						printf("\n\t");
						missX = 0;
						missY = 0;
						for(z = 0; z < temp->traceLength; z++)
	                                        {
							if(z % 59 == 0 && z != 0)
	                                                        printf("\n");
	                                                else
	                                                {
								if(temp->tb[z] == 'X')
								{
									printf(" ");
									missX++;
								}
								else if(temp->tb[z] == 'Y')
	                                                        {
	                                                                printf(" ");
	                                                                missY++;
	                                                        }
								else if(temp->tb[z] == 'U')
									printf(" ");
	                                                        else if(temp->tb[z] == 'M')
	                                                                printf("|");
								else
									printf(" ");
							}
						}
						printf("\nQuery\t");
						miss = 0;
						for(z = 0; z < temp->traceLength; z++)
        	                                {
							if(z % 59 == 0 && z != 0)
        	                                                printf("\n");
        	                                        else
        	                                        {
		                                                if(temp->tb[z] == 'M' || temp->tb[z] == 'U')
		                                                        printf("%c",query.sequence[i][z-miss]);
		                                                else if(temp->tb[z] == 'X')
									printf("%c",query.sequence[i][z-miss]);
		                                                else if(temp->tb[z] == 'Y')
		                                                {
									printf("-");
									miss++;
		                                                }
							}
	                                        }
	
						printf("\n\nStart: %d\nEnd: %d\n\n-----------------------------------------------\n\n",index[j].SA[temp->low],index[j].SA[temp->low]+temp->traceLength-1);
						temp = temp->next;
					}
					else
						temp = temp->next;
				}
			}
			/*else
				printf("No Matches Found\n\n-----------------------------------------------\n\n\n\n");*/
		}
	}	
}

/*

	printKLs

	prints the calculated K and L values from each input/index combination

	out: 2D array of structures that contain search output information

	qsc: integer variable containing the number of query sequences

	isc: integer variable containing the number of index sequences

*/

void 
printKLs(struct results **out, int qsc, int isc)
{
	int i,j;
	struct matches *temp;
	for(i = 0; i < qsc; i ++)
	{
		for(j = 0; j < isc; j++)
		{
			printf("index: %d\tquery: %d\n", j+1, i+1);
			temp = (struct matches *) malloc(sizeof(struct matches));
                        temp = out[i][j].match;
			if(temp != NULL)
                        {
                                while(temp != NULL)
					printf("k: %d\tl: %d\n", temp->low, temp->high);
			}
		}
	}
}

/*				*
 * 	MAIN FUNCTION		*
 * 				*/

int 
main(int argc, char *argv[])
{
	int QseqCount = 0;
	int IseqCount = 0;
	int ***D;
	int ***S; 
        struct results **out;
	struct input query;
	struct FMidx *index;
	createIndexHash();
	createBaseHash();
	query = manageInputs(argv,argc,&QseqCount);
        index = getIndex();
	IseqCount = getCount();
	if(subMatType)
		readSubMat(subMatType);

	/*Search*/
	if(searchType > 0)
	{
		D = calculateD(index,IseqCount,query,QseqCount);
		if(searchType == 1)
			out = distanceSearch(query,QseqCount,index,IseqCount,D);
		else
			out = conservedSearch(query,QseqCount,index,IseqCount,D);
	}
	else
	{
		S = calculateS(index,IseqCount,query,QseqCount);
		out = scoredSearch(query,QseqCount,index,IseqCount,S);
	}

	/*Output*/
	if(outputFile)
		outputToFile(out,QseqCount,IseqCount,index,query);
	if(verbose)
		printKLs(out,QseqCount,IseqCount);
	if(!silent)
		printInResults(out,QseqCount,IseqCount,index,query);
	return 0;
}
