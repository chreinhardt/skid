#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <assert.h>
#include <fenv.h>
#include "kd.h"
#include "smooth1.h"


#define PRUNE_STEPS		5
#define MICRO_STEP		0.1


void usage(void)
{
	fprintf(stderr,"USAGE:\n");
	fprintf(stderr,"skid -tau <fLinkLength> [OPTIONAL ARGUMENTS]\n");
	fprintf(stderr,"	 reads TIPSY BINARY input file from stdin\n");
	fprintf(stderr,"     [-std]\n");
	fprintf(stderr,"COSMOLOGY and UNITS arguments:\n");
	fprintf(stderr,"     [-z <fRedShift>] [-O <fOmega>]\n");
	fprintf(stderr,"     [-G <fGravConst>] [-H <fHubble>]\n");
	fprintf(stderr,"     [-Lambda <fLambda>] [-Q <fQuintessense]\n");
	fprintf(stderr,"GROUP FINDING arguments (see man page!):\n");
	fprintf(stderr,"     [-s <nSmooth>] [-d <fMinDensity>] [-t <fMaxTemp>]\n");
	fprintf(stderr,"     [-cvg <fConvergeRadius>] [-scoop <fScoopRadius>]\n");
	fprintf(stderr,"     [-m <nMinMembers>] [-nu] [-gd] [-unbind <GroupName>[.grp]]\n");
	fprintf(stderr,"     [-M <fMaxMass>] [-fic] [-go] [-maxgroup nMaxMembers]\n");
	fprintf(stderr,"GRAVITATIONAL SOFTENING arguments:\n");
	fprintf(stderr,"     [-spline] [-plummer] [-e <fSoft>]\n"); 
	fprintf(stderr,"PERIODIC BOX specification:\n");
	fprintf(stderr,"     [-p <xyzPeriod>]\n");
	fprintf(stderr,"     [-c <xyzCenter>]\n");
	fprintf(stderr,"     [-cx <xCenter>] [-cy <yCenter>] [-cz <zCenter>]\n");
	fprintf(stderr,"OUTPUT arguments:\n");
	fprintf(stderr,"     [-o <Output Name>] [-ray] [-den] [-stats] [-diag]\n");
	fprintf(stderr,"\nSee man page skid(1).\n");
	exit(1);
	}

int main(int argc,char **argv)
{
	KD kd;
	SMX smx;
	/*
	 ** Input argument variables and control.
	 */
	int bTau,bCvg,bScoop,nSmooth,nMembers,bNoUnbind,bUnbindOnly,iSoftType;
	int bEps,bOutRay,bOutDens,bGasAndDark,bGasOnly,bOutStats,bForceInitialCut;
	int nMaxMembers;
	int bPeriodic;
	int bStandard;
	float fTau,z,Omega0,G,H0,fDensMin,fTempMax,fMassMax,fCvg,fScoop,fEps;
	float Lambda;		/* Cosmological constant */
	float fQuintess;	/* Omega_quintessence */
	float fPeriod[3],fCenter[3];
	char achGroup[256],achName[256];
	/*
	 ** Working variables for SKID mainline.
	 */
	float fStep;
	int nBucket,i,j,nActive,nIttr;
	int sec1,usec1;
	int sec2,usec2;
	int sec3,usec3;
	int sec4,usec4;
	int sec5,usec5;
	char achFile[256];
	int iExt;
	int nScat;
	int bOutDiag;

	feenableexcept(FE_OVERFLOW | FE_DIVBYZERO | FE_INVALID);

	printf("SKID v1.4.1: Joachim Stadel, Dec. 2000\n");
	/*
	 ** Bucket size set to 16, user cannot affect this!
	 */
   	nBucket = 16;
	/*
	 ** bTau flag to make sure user has at least specified the -tau argument.
	 */
	bTau = 0;
	/*
	 * Default tipsy native format.
	 */
	bStandard = 0;
	/*
	 ** Default Cosmological parameters.
	 */
	z = 0.0;
	Omega0 = 1.0;
	Lambda = 0.0;
	fQuintess = 0.0;
	G = 1.0;
	H0 = 0.0;
	/*
	 ** Default group finding parameters (those not dependent on fTau).
	 */
	nSmooth = 64;
	fDensMin = 0.0;
	fTempMax = FLT_MAX;
	fMassMax = FLT_MAX;
	bCvg = 0;
	bScoop = 0;
	nMembers = 8;
	nMaxMembers = INT_MAX;
	bNoUnbind = 0;
	bGasAndDark = 0;
	bGasOnly = 0;
	bUnbindOnly = 0;
	bForceInitialCut = 0;
	bPeriodic = 0;
	/*
	 ** Default gravitational parameters.
	 */
	bEps = 0;
	iSoftType = SPLINE;
	/*
	 ** Default periodic box parameters.
	 */
	for (j=0;j<3;++j) {
		fPeriod[j] = FLT_MAX;
		fCenter[j] = 0.0;
		}
	/*
	 ** Default output parameters.
	 */
	strcpy(achName,"skid");
	bOutRay = 0;
	bOutDens = 0;
	bOutStats = 0;
	bOutDiag = 0;
	/*
	 ** Now get the command line arguments!
	 */
	i = 1;
	while (i < argc) {
		if (!strcmp(argv[i],"-tau")) {
			++i;
			if (i >= argc) usage();
			fTau = atof(argv[i]);
			bTau = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-z")) {
			++i;
			if (i >= argc) usage();
			z = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-O")) {
			++i;
			if (i >= argc) usage();
			Omega0 = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-Lambda")) {
			++i;
			if (i >= argc) usage();
			Lambda = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-Q")) {
			++i;
			if (i >= argc) usage();
			fQuintess = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-G")) {
			++i;
			if (i >= argc) usage();
			G = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-H")) {
			++i;
			if (i >= argc) usage();
			H0 = atof(argv[i]);
			++i;
			}
	    else if (!strcmp(argv[i],"-s")) {
			++i;
			if (i >= argc) usage();
			nSmooth = atoi(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-d")) {
			++i;
			if (i >= argc) usage();
			fDensMin = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-t")) {
			++i;
			if (i >= argc) usage();
			fTempMax = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-M")) {
			++i;
			if (i >= argc) usage();
			fMassMax = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-fic")) {
			bForceInitialCut = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-cvg")) {
			++i;
			if (i >= argc) usage();
			fCvg = atof(argv[i]);
			bCvg = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-scoop")) {
			++i;
			if (i >= argc) usage();
			fScoop = atof(argv[i]);
			bScoop = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-m")) {
			++i;
			if (i >= argc) usage();
			nMembers = atoi(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-maxgroup")) {
			++i;
			if (i >= argc) usage();
			nMaxMembers = atoi(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-nu")) {
			bNoUnbind = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-gd")) {
			bGasAndDark = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-go")) {
			bGasOnly = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-unbind")) {
			++i;
			if (i >= argc) usage();
			strcpy(achGroup,argv[i]);
			bUnbindOnly = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-spline")) {
			iSoftType = SPLINE;
			++i;
			}
		else if (!strcmp(argv[i],"-plummer")) {
			iSoftType = PLUMMER;
			++i;
			}
		else if (!strcmp(argv[i],"-e")) {
			++i;
			if (i >= argc) usage();
			fEps = atof(argv[i]);
			bEps = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-p")) {
			++i;
			if (i >= argc) usage();
			fPeriod[0] = atof(argv[i]);
			fPeriod[1] = atof(argv[i]);
			fPeriod[2] = atof(argv[i]);
			bPeriodic = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-c")) {
			++i;
			if (i >= argc) usage();
			fCenter[0] = atof(argv[i]);
			fCenter[1] = atof(argv[i]);
			fCenter[2] = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-cx")) {
			++i;
			if (i >= argc) usage();
			fCenter[0] = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-cy")) {
			++i;
			if (i >= argc) usage();
			fCenter[1] = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-cz")) {
			++i;
			if (i >= argc) usage();
		    fCenter[2] = atof(argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-o")) {
			++i;
			if (i >= argc) usage();
			strcpy(achName,argv[i]);
			++i;
			}
		else if (!strcmp(argv[i],"-ray")) {
			bOutRay = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-den")) {
			bOutDens = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-stats")) {
			bOutStats = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-diag")) {
			bOutDiag = 1;
			++i;
			}
		else if (!strcmp(argv[i],"-std")) {
		        bStandard = 1;
			++i;
		        }
		else usage();
		}
	/*
	 ** Make sure user has specified the -tau argument.
	 */
	if (!bTau) usage();
	/*
	 ** Default other parameters if needed.
	 */
	if (!bCvg) fCvg = 0.5*fTau;
	if (!bScoop) fScoop = 2.0*fTau;
	fStep = 0.5*fCvg;

	kdInit(&kd,nBucket,fPeriod,fCenter,bOutDiag);
	kdReadTipsy(kd,stdin, bStandard);
	if (bUnbindOnly) {
		/*
		 ** to provide compatibility with v1.2 skid we need to strip off
		 ** the .grp extension if it is specified.
		 */
		iExt = strlen(achGroup) - 4;
		if (iExt < 0) iExt = 0;
		if (!strcmp(&achGroup[iExt],".grp")) {
			/*
			 ** The extension was provided, so remove it.
			 */
			achGroup[iExt] = 0; 
			}
		strcpy(achFile,achGroup);
		strcat(achFile,".grp");
		kdInGroup(kd,achFile);
		/*
		 ** Check and see if there exists a .gtp file with the right name.
		 */
		strcpy(achFile,achGroup);
		strcat(achFile,".gtp");
		kdInitpGroup(kd);
		kdReadCenter(kd,achFile, bStandard);
		goto UnbindOnly;
		}
	kdScatterActive(kd,bGasAndDark,bGasOnly);
	kdBuildTree(kd);
	smInit(&smx,kd,nSmooth);
	kdTime(kd,&sec1,&usec1);
	smDensityInit(smx,bPeriodic);
	kdTime(kd,&sec1,&usec1);
	/*
	 ** Output density if requested.
	 */
	if (bOutDens) {
		strcpy(achFile,achName);
		strcat(achFile,".den");
		kdOutDensity(kd,achFile);
		}
	/*
	 ** Find initial moving particles.
	 ** Move the moving particles for one step and reduce the
	 ** number of scatterers if possible.
	 */
	kdTime(kd,&sec2,&usec2);
	nActive = kdInitMove(kd,fDensMin,fTempMax,fMassMax,fCvg,bGasAndDark,bGasOnly);
	kdBuildMoveTree(kd);
	if (bAllowInitialCut(kd) || bForceInitialCut) { 
		nScat = smAccDensity(smx,1);
		}
	else {
		nScat = smAccDensity(smx,0);
		}
	printf("Ittr:%d nActive:%d nScatter:%d\n",0,nActive,nScat);
	fflush(stdout);
	kdMoveParticles(kd,fStep);
	/*
	 ** Do the main "flow" loop for the moving particles.
	 */
	nIttr = 1;
	while (nActive) {
		for (i=0;i<PRUNE_STEPS;++i) {
			kdBuildMoveTree(kd);
			nScat = smAccDensity(smx,0);
			kdMoveParticles(kd,fStep);
			}
		nActive = kdPruneInactive(kd,fCvg);
		printf("Ittr:%d nActive:%d nScatter:%d\n",nIttr,nActive,nScat);
		fflush(stdout);
		++nIttr;
		}
	kdTime(kd,&sec2,&usec2);
	/*
	 ** Assign groups using the friends-of-friends algorithm
	 */
	kdTime(kd,&sec3,&usec3);
	kdFoF(kd,fTau);
	kdTime(kd,&sec3,&usec3);
	/*
	 ** Micro-stepping phase.
	 */
	kdTime(kd,&sec5,&usec5);
	kdReactivateMove(kd);
	for (i=0;i<PRUNE_STEPS;++i) {
		kdBuildMoveTree(kd);
		nScat = smAccDensity(smx,0);
		kdMoveParticles(kd,MICRO_STEP*fStep);
		printf("Microstep:%d nScatter:%d\n",i+1,nScat);
		fflush(stdout);
		}
	kdTime(kd,&sec5,&usec5);
	smFinish(smx);
	/*
	 ** Output the .ray file if requested (TIPSY VECTOR format).
	 */
	if (bOutRay) {
		strcpy(achFile,achName);
		strcat(achFile,".ray");
		kdOutVector(kd,achFile);
		}
	/*
	 ** Prepare for unbinding.
	 */
	kdInitpGroup(kd);
	kdCalcCenter(kd);
 UnbindOnly:
	/*
	 ** Set the cosmological parameters.  These are also needed in
	 ** kdOutStats().
	 */
	kdSetUniverse(kd,G,Omega0,Lambda,fQuintess,H0,z);
	/*
	 ** Set the softening of all particles, if a softening was 
	 ** specified.
	 */
	if (bEps) kdSetSoft(kd,fEps);
	/*
	 ** Do the unbinding of particles.
	 */
	kdTime(kd,&sec4,&usec4);
	kdUnbind(kd,iSoftType,fScoop,bGasAndDark,bNoUnbind,nMaxMembers);
	kdTime(kd,&sec4,&usec4);
	kdTooSmall(kd,nMembers);
	/*
	 ** Output group files.
	 */
	strcpy(achFile,achName);
	strcat(achFile,".grp");
	kdOutGroup(kd,achFile);
	strcpy(achFile,achName);
	strcat(achFile,".gtp");
	kdWriteGroup(kd,achFile, bStandard);
	if (bOutStats) {
	    strcpy(achFile,achName);
	    strcat(achFile,".stat");
	    kdOutStats(kd,achFile,fDensMin,fTempMax);
		}
	printf("SKID CPU Time:\n");
	if (!bUnbindOnly) {
		printf("   Initial Density:    %d.%06d\n",sec1,usec1);
		printf("   Moving Particles:   %d.%06d\n",sec2,usec2);
		printf("   Friends of Friends: %d.%06d\n",sec3,usec3);
		printf("   Microstepping:      %d.%06d\n",sec5,usec5);
		}
	if (!bNoUnbind) printf("   Unbinding:          %d.%06d\n",sec4,usec4);
	fflush(stdout);
	kdFinish(kd);
	return 0;
	}
	

