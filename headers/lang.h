#ifndef LANG_H
#define LANG_H
enum LangIDs {
	LANG_SUCC,
	LANG_UNKERR,

	LANG_SVSTART,
	LANG_SVFULL,
	LANG_SVMANYCONN,
	LANG_SVPLCONN,
	LANG_SVPLDISCONN,
	LANG_SVPLUGINLOAD,
	LANG_HBEAT,
	LANG_SVDELTALT0,
	LANG_SVLONGTICK,
	LANG_SVLOOPDONE,
	LANG_SVSTOP0,
	LANG_SVSTOP1,
	LANG_SVSTOP2,
	LANG_SVLOADING,
	LANG_SVSAVING,

	LANG_KICKNOREASON,
	LANG_KICKPROTOVER,
	LANG_KICKNAMEUSED,
	LANG_KICKAUTHFAIL,
	LANG_KICKSVSTOP,
	LANG_KICKMAPFAIL,
	LANG_KICKPACKETREAD,
	LANG_KICKBLOCKID,
	LANG_KICKPACKETSPAM,

	LANG_PLNAMEUNK,

	LANG_CPEVANILLA,

	LANG_CMDUSAGE,
	LANG_CMDAD,
	LANG_CMDPLNF,
	LANG_CMDUNK,
	LANG_CMDONLYCL,
	LANG_CMDONLYCON,

	LANG_CPLD,
	LANG_CPUNLD,
	LANG_CPCB,
	LANG_CPLDS,
	LANG_CPNL,
	LANG_CPINF0,
	LANG_CPINF1,
	LANG_CPLIST,
	LANG_CPINVNAME,

	LANG_DBGSYM0,
	LANG_DBGSYM1,

	LANG_HBRESPERR,
	LANG_HBRR,
	LANG_HBRSP,
	LANG_HBERR,
	LANG_HBPLAY,

	LANG_WSNOTVALID,

	LANG_ERRFMT
};

API const char* Lang_Get(int langid);
#endif
