/*
 * Copyright (C) 2015-2021 IoT.bzh Company
 * Author "Fulup Ar Foll"
 *
 * $RP_BEGIN_LICENSE$
 * Commercial License Usage
 *  Licensees holding valid commercial IoT.bzh licenses may use this file in
 *  accordance with the commercial license agreement provided with the
 *  Software or, alternatively, in accordance with the terms contained in
 *  a written agreement between you and The IoT.bzh Company. For licensing terms
 *  and conditions see https://www.iot.bzh/terms-conditions. For further
 *  information use the contact form at https://www.iot.bzh/contact.
 *
 * GNU General Public License Usage
 *  Alternatively, this file may be used under the terms of the GNU General
 *  Public license version 3. This license is as published by the Free Software
 *  Foundation and appearing in the file LICENSE.GPLv3 included in the packaging
 *  of this file. Please review the following information to ensure the GNU
 *  General Public License requirements will be met
 *  https://www.gnu.org/licenses/gpl-3.0.html.
 * $RP_END_LICENSE$
 * 
 * references:
 *  Vector data model https://gdal.org/user/vector_data_model.html
 *  Vector C++ api https://gdal.org/tutorials/vector_api_tut.html
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <ogrsf_frmts.h>

#include "misc-utils.h"

static struct option options[] = {
	{"verbose", optional_argument , 0, 'v' },
	{"force"  , optional_argument , 0, 'f' },
	{"config" , optional_argument , 0, 'c' },
	{"inpath" , optional_argument , 0, 'i' },
	{"outpath", optional_argument , 0, 'o' },
	{"list"   , optional_argument, 0,  'l' },
	{"outfmt" , optional_argument, 0,  'd' },
	{"help"   , optional_argument , 0, 'h' },
	{0, 0, 0, 0 } // trailer
};

typedef struct {
    const char*config;
    const char*inpath;
    const char*outpath;
    const char*outfmt;
    int verbose;
    int index;
    int listD;
    int force;
} clientArgsT;

static clientArgsT *parseArgs(int argc, char *argv[]) {
	clientArgsT *opts = (clientArgsT*)calloc (1, sizeof(clientArgsT));
 	int index;

    if (argc < 2) goto OnErrorExit;

	for (int done=0; !done;) {
		int option = getopt_long(argc, argv, "", options, &index);
		if (option == -1) {
			opts->index= optind;
			break;
		}

		// option return short option even when long option is given
		switch (option) {
			case 'v':
				opts->verbose++;
  				if (optarg)	opts->verbose = atoi(optarg);
				break;

			case 'i':
				opts->inpath= optarg;
				break;

			case 'f':
				opts->force= 1;
				break;

			case 'l':
				opts->listD=1;
				break;

			case 'd':
				opts->outfmt= optarg;
				break;

			case 'o':
				opts->outpath= optarg;
				break;

			case 'h':
			default:
				goto OnErrorExit;
		}
	}

    return opts;

OnErrorExit:
	fprintf (stderr, "usage: gdal-client --inpath=/xxx/my-config.xxx --outpath=xxxx [--verbose] [--list] [--outfmt='MVT']\n");
	return NULL;
}

static void dumpLayer (OGRLayer* inLayer, int verbose) {

    // get input layer definitions
    OGRFeatureDefn *inLayerDefn = inLayer->GetLayerDefn();
    const char *lname= inLayer->GetName();
    OGRwkbGeometryType lType= inLayer->GetGeomType();

    const char* ltype= OGRGeometryTypeToName(lType);
    if (verbose) fprintf (stderr, "\n-- layer=%s type=%s\n", lname, ltype);

    // loop on features
    while(OGRFeature *poFeature= inLayer->GetNextFeature()) {
        
        const OGRGeometry *poGeometry = poFeature->GetGeometryRef();
        if( poGeometry != nullptr && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint ) {
            const OGRPoint *poPoint = poGeometry->toPoint();

            if (verbose) fprintf (stderr, " --- Feature geometry: %.3f,%3.f ", poPoint->getX(), poPoint->getY());
        } else {
            if (verbose) fprintf(stderr, " --- feature no point geometry ");
        }

        for( int iField = 0; iField < inLayerDefn->GetFieldCount(); iField++ ) {
            OGRFieldDefn *poFieldDefn = inLayerDefn->GetFieldDefn(iField);

            switch( poFieldDefn->GetType()) {
                case OFTInteger:
                    if (verbose >1) fprintf(stderr, "int: %d,", poFeature->GetFieldAsInteger( iField ) );
                    break;
                case OFTInteger64:
                    printf( CPL_FRMT_GIB ",", poFeature->GetFieldAsInteger64( iField ) );
                    break;
                case OFTReal:
                    if (verbose >1) fprintf(stderr, "float: %.3f,", poFeature->GetFieldAsDouble(iField) );
                    break;
                case OFTString:
                    if (verbose >1) fprintf(stderr, "string: '%s',", poFeature->GetFieldAsString(iField)  );
                    break;
                default:
                    fprintf(stderr, "unknown '%s',", poFeature->GetFieldAsString(iField)  );
                    break;
            }
        }

        if (verbose) fprintf (stderr, "\n"); 
    } // end feature
}

int main(int argc, char *argv[]) {
    GDALDataset *inDS, *outDS=NULL;
    GDALDriverManager *gManager;
    char **papszOptions;
    int err;

    clientArgsT *opts= parseArgs (argc, argv);
    if (!opts) goto OnErrorExit;

    // initialise lib
    GDALAllRegister();

    gManager= GetGDALDriverManager();

    if (opts->listD) {
        for (int idx=0; idx < gManager->GetDriverCount(); idx++) {
            GDALDriver *outDriver= gManager->GetDriver(idx);
            printf( "--driver[%d]=%s\n", idx, outDriver->GetDescription());
        }
    }

    // set S57 reader options
    papszOptions=NULL;
    papszOptions= CSLSetNameValue(papszOptions,"SPLIT_MULTIPOINT", "ON");
    papszOptions= CSLSetNameValue(papszOptions,"ADD_SOUNDG_DEPTH", "ON");
    papszOptions= CSLSetNameValue(papszOptions,"RECODE_BY_DSSI", "ON");

    // open input chart
    inDS= (GDALDataset*) GDALOpenEx(opts->inpath, GDAL_OF_VECTOR, NULL, papszOptions, NULL );
    if(!inDS) {
        printf( "Open failed mapfile=%s\n", opts->inpath);
        goto OnErrorExit;
    }

     if (opts->outpath) {

        // force flag clean up destination directory when exit 
        if (opts->force) {
            err= UtilsRemoveDir (opts->outpath, RMDIR_PARENT_DEL);
            if (err < 0) goto OnErrorExit;
        }

        // open output driver
        //const char *pszDriverName = "MVT";
        if (!opts->outfmt) opts->outfmt= "ESRI Shapefile";
        GDALDriver *outDriver = GetGDALDriverManager()->GetDriverByName(opts->outfmt);
        if( outDriver == NULL ) {
            fprintf(stderr, "%s driver not available.\n", opts->outfmt);
            goto OnErrorExit;
        }

        outDS = outDriver->Create(opts->outpath, 0, 0, 0, GDT_Unknown, NULL );
        if (outDS == NULL) {
            fprintf(stderr, "Fail to create output file=%s \n", opts->outpath);
           goto OnErrorExit;
        }
    } 

    // loop on chart layers
    for (OGRLayer* inLayer: inDS->GetLayers()) {
        const char *lname= inLayer->GetName();
        
        if (opts->verbose) dumpLayer (inLayer, opts->verbose);

        // duplicate input layer input output driver
        if (outDS) outDS->CopyLayer(inLayer, lname, NULL);

    } // end layer

    // write converted chart
    if (outDS)  GDALClose(outDS);

    return 0;

OnErrorExit: 
    fprintf (stderr, "OnErrorExit [try gdal-client --help]\n");  
    return 1;
}