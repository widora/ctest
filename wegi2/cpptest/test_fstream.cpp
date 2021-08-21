/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string.h>
using namespace std;

int main(int argc, char **argv)
{
	ifstream fin;
	char *fpath=argv[1];
	char ch;
//	int i;

        fin.open(fpath);
        if(fin.fail()) {
                cout <<"Fail to open " <<fpath <<endl;
                return -1;
        }

	int index;
	int off;

	int vcnt=0;
	int tcnt=0;
	/* TEST: 	----- getline -----
	 *  istream & geline (char *s, streamsize n);
	 *  istream & geline (char *s, streamsize n, char delim);
	    Note:
		1. getline() will NOT save '\n' OR delim !!!
	*/
        int             k;                  /* triangle index in a face, [0 3] */
	int		m;		    /* 0: vtxIndex, 1: texutreIndex 2: normalIndex  */
        int             vtxIndex[4]={0};    /* To store vtxIndex for 4_side face */
        int             textureIndex[4]={0};
        int             normalIndex[4]={0};
        char *savept;
        char *savept2;
	char *pt, *pt2;
	char strline[1024];
	int n=0;
	while( fin.getline(strline, 1024-1) ) {
		/* Read obj face */
		if(strline[0]=='f'){
			cout << "face line: "<< strline <<endl;
			n++;

		   /* Case A. --- ONLY vtxIndex
		    * Example: f 61 21 44
		    */
		   if( strstr(strline, "/")==NULL ) {
			/* To extract face vertex indices */
			pt=strtok(strline+1, " ");
			for(k=0; pt!=NULL && k<4; k++) {
				vtxIndex[k]=atoi(pt) -1;
				pt=strtok(NULL, " ");
			}

			/* Count triangle */
			tcnt += (k>2)?(k-2):0;

			/* Print all vtxIndex[] */
			for(int i=0; i<k; i++)
				printf("vtxIndex[%d]=%d ", i, vtxIndex[i]);
			printf("\n");

		   }
		   /* Case B. --- NOT only vtxIndex, maybe ALSO with textureIndex,normalIndex, with delim '/',
		    * Example: f 69//47 68//47 43//47 52//47
		    */
		   else {

			/* To replace "//" with "/0/", to avoid strtok() returns only once!
			 * However, it can NOT rule out conditions like "/0/5" and "5/0/",
			 * see following  "In case "/0/5" ..."
			 */
			while( (pt=strstr(strline, "//")) ) {
				int len;
				len=strlen(pt+1)+1; /* len: from second '/' to EOF */
				memmove(pt+2, pt+1, len);
				*(pt+1) ='0'; /* insert '0' btween "//" */
			}

			/* To extract face vertex indices */
			pt=strtok_r(strline+1, " ", &savept);
			for(k=0; pt!=NULL && k<4; k++) {
				cout << "Index group: " << pt<<endl;
				/* Parse vtxIndex/textureIndex/normalIndex */
				pt2=strtok_r(pt, "/", &savept2);
				for(m=0; pt2!=NULL && m<3; m++) {
					cout <<"pt2: "<<pt2<<endl;
					switch(m) {
					   case 0:
						vtxIndex[k]=atoi(pt2)-1;
						break;
					   case 1:
						textureIndex[k]=atoi(pt2)-1;
						break;
					   case 2:
						normalIndex[k]=atoi(pt2)-1;
					}


					/* Next param */
					pt2=strtok_r(NULL, "/", &savept2);
				}
				/* In case  "/0/5", -- Invalid, vtxIndex MUST NOT be omitted! */
				if(pt[0]=='/' ) {
					cout<< "!!!WARNING!!!  vtxIndex omitted!\n";
					/* Reorder */
					normalIndex[k]=textureIndex[k];
					textureIndex[k]=vtxIndex[k];
					vtxIndex[k]=-1;
				}
				/* In case "5/0/", -- m==2, normalIndex omitted!  */
				else if( m==2 ) { /* pt[0]=='/' also has m==2! */
					normalIndex[k]=0-1;
				}

				/* Get next indices */
				pt=strtok_r(NULL, " ", &savept);
			}

			/* Count triangle */
			tcnt += (k>2)?(k-2):0;

			/* Print all indices */
			printf("vtx/texture/normal Index: ");
			for(int i=0; i<k; i++)
				printf(" %d/%d/%d ", vtxIndex[i], textureIndex[i], normalIndex[i]);
			printf("\n");

		   }

		}
		else
			cout << "Not face line: " <<strline<<endl;



	} /* End fin.getline */

	cout <<"Total " <<n << " face data lines"<<endl;
	//cout <<"Vertices: " << vcnt <<endl;
	cout <<"Triangles: " << tcnt <<endl;
	exit(0);

//////////////////////////////////////////////////////////////////

	/* Get rid of "f " */
	fin.seekg(2, ios::cur);

	ch=0;
/* A Face has MAX. 4 vertices */
for( int k=0; k<4; k++) {
	/* Example: f 69//47 68//47 43//47 52//47 */
#if 1	/* Check line END */
	if( ch=='\n' || ch=='\r' )
		break;
	else {
		off=fin.tellg();
		fin.get(ch);
		while( ch==' ' ){ fin.get(ch); };
		if( ch=='\n' || fin.eof() ) {
			cout << "End of face line  " <<k <<endl;
			break;	/* END of line */
		}
		else {
			cout << "ch: " << ch <<endl;
			fin.seekg(off, ios::beg);
		}
	}
#endif
	cout<<"k= "<<k<<endl;

	/* 1. [VtxIndex] MUST be at first */
	fin >>index; cout <<index <<"_";

	/* 2. Read out first '/' */
	//fin >>ch;    //cout <<ch <<"_";
	fin.get(ch);

	/* 3. [textureIndex] ONLY if not second '/' */
	//fin >>ch;
	fin.get(ch);
	if(ch != '/') {
		fin.seekg(-1, ios::cur);
		fin >>index;  cout <<index <<"_";
	}
	else {  /* IS '/', textureIndex NOT exists! */
		index=0;
		cout <<index <<"_";
	}

        /* 4. [normalIndex]  */
        //fin >>ch;  /* Will get rid of '\n' */
	fin.get(ch);
	/* 4.0 If EOL: Check at loop beginning... */
	/* 4.1 ONLY If textrueIndex exists at 3. */
	if( ch == '/' ) {
		fin >>index;  cout <<index <<"_";
	}
	/* 4.2 If textrueIndex NOT exist at 3. */
	else if ( ch != ' ' ) {
		fin.seekg(-1, ios::cur);
		fin >>index; cout <<index <<"_";
	}
	else {   // ch==' ' normalIndex NOT exists!
		index=0;
		cout <<index <<"_";
	}
	cout <<endl;

	cout <<"last ch: "<< int(ch) <<endl;

}



#if 1
	while( !fin.eof() && fin.get(ch) )
		cout << ch;
#endif
	cout<<endl;

	fin.close();
}
