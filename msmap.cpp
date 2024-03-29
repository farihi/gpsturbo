/*********************************************************************************/
/* GPSTurbo                                                                      */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/*                                                                               */
/* http://www.scale18.com/cgi-bin/page/gpsturbo.html                             */
/*                                                                               */
/*    GPSTurbo is free software; you can redistribute it and/or modify           */
/*    it under the terms of the GNU General Public License as published by       */
/*    the Free Software Foundation; version 2.                                   */
/*                                                                               */
/*    GPSTurbo is distributed in the hope that it will be useful,                 */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/*    GNU General Public License for more details.                               */
/*                                                                               */
/*    http://www.gnu.org/licenses/gpl.txt                                        */
/*                                                                               */
/*    You should have received a copy of the GNU General Public License          */
/*    along with GPSTurbo; if not, write to the Free Software                    */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */
/*                                                                               */
/*********************************************************************************/

#include "gpsturbo.h"
#include "heap.h"

/* overedraw by this many pixels since streets have width and are not single lines */
#define CLIPEXPAND 20

Hash *MSGPXFName::m_hash=0;

unsigned int MSGPXMap::m_numdrawsubs;
kGUICorners MSGPXMap::m_tilecorners;
int MSGPXMap::m_tx;
int MSGPXMap::m_ty;
Array<MSGPXMap *> MSGPXMap::m_drawmaps;
Array<MSSUBDIV *> MSGPXMap::m_drawsubdivs;
MSCOORD MSGPXMap::m_points[MAXPP];
kGUIFPoint2 MSGPXMap::m_ppoints[MAXPP];

unsigned int MSGPXMap::m_numsortpolys;
Array<POLYSORT_DEF>MSGPXMap::m_sortpolys;

int MSGPXMap::m_numiconsdrawn;
Array<ICON_POS> MSGPXMap::m_iconpos;

unsigned int MSGPXMap::m_roadgroupspolys[ROADGROUP_NUM];
Array<POLYSORT_DEF>MSGPXMap::m_roadgroups[ROADGROUP_NUM];

Heap MSGPXMap::m_sortpolysheap;

GPXMapStrings MSGPXMap::m_lc;
//kGUIDrawSurface *MSGPXMap::m_lcwindow=0;	/* label collision window */
//kGUICorners MSGPXMap::m_lcbounds;

int MSGPXMap::m_labelicon;	/* icon number for last loaded label */
Array<kGUIImage *>MSGPXMap::m_icons;

const char *iconnames[6]={
	"icon1.gif",
	"icon2.gif",
	"icon3.gif",
	"icon4.gif",
	"icon5.gif",
	"icon5.gif"};

int iconcenterx[6]={
	10,
	11,
	12,
	12,
	13,
	13,
};
	
int iconcentery[6]={
	12,
	9,
	11,
	9,
	8,
	8,
};
	
void MSGPXMap::Init(void)
{
	unsigned int i;
	kGUIImage *image;

	m_lc.Init(256,256);

	m_drawsubdivs.Init(50,25);
	m_drawmaps.Init(50,25);
	m_sortpolys.Init(1024,256);
	m_sortpolysheap.SetBlockSize(65536);
	for(i=0;i<ROADGROUP_NUM;++i)
		m_roadgroups[i].Init(1024,256);

	m_iconpos.Init(16,4);
	m_icons.Alloc(6);	/* 0x2a to 0x2f */
	for(i=0;i<6;++i)
	{
		image=new kGUIImage();
		m_icons.SetEntry(i,image);
//		image->SetRaw(iconptrs[i]);
		image->SetFilename(iconnames[i]);
	}
}

void MSGPXMap::Purge(void)
{
	int i;

	for(i=0;i<6;++i)
		delete m_icons.GetEntry(i);
}

void MSGPXFName::Load(const char *path)
{
	int i,numfiles;
	kGUIString *s;
	kGUIString id;
	const char *fn;
	kGUIDir dir;

	if(!m_hash)
	{
		m_hash=new Hash();
		m_hash->Init(12,sizeof(kGUIString *));
	}

	dir.LoadDir(path,true,true,".img");

	numfiles=dir.GetNumFiles();
	for(i=0;i<numfiles;++i)
	{
		fn=dir.GetFilename(i);
		s=new kGUIString();
		s->SetString(fn);
		id.Sprintf("%d",atoi(fn+strlen(fn)-(8+4)));
		if(!(m_hash->Find(id.GetString())))
			m_hash->Add(id.GetString(),&s);
		else
			delete s;
	}
}

void MSGPXFName::Purge()
{
	HashEntry *he;
	int h,hh;
	kGUIString **s;

	if(m_hash)
	{
		/* free strings */
		he=m_hash->GetFirst();
		hh=m_hash->GetNum();
		for(h=0;h<hh;++h)
		{
			s=(kGUIString **)he->m_data;
			delete *s;
			he=he->GetNext();
		}

		/* free all strings */
		delete m_hash;
	}
}

const char *MSGPXFName::GetFName(int mapnum)
{
//	int i,numfiles;
	kGUIString id;
	kGUIString **s;

	id.Sprintf("%d",mapnum);
	s=(kGUIString **)m_hash->Find(id.GetString());
	if(s)
		return((*s)->GetString());
	return(0);
}

/* static function */
void MSGPXMap::GetLongName(const char *fn,kGUIString *longname)
{
	kGUIString tdbfn;
	DataHandle dh;

	longname->Clear();
	tdbfn.SetString(fn);
	tdbfn.Replace(".img",".tdb");

	dh.SetFilename(tdbfn.GetString());
	if(dh.Open())
	{
		int blocklen;
		unsigned char blockid;
		unsigned char v1;
		unsigned char v2;
		unsigned long long index;

		index=0;
		while(index<dh.GetSize())
		{
			dh.Seek(index);
			dh.Read(&blockid,(unsigned long long)1L);
			dh.Read(&v1,(unsigned long long)1L);
			dh.Read(&v2,(unsigned long long)1L);
			blocklen=((int)v1|((int)v2<<8));
			index+=3;

			if(blockid==0x50)
			{
				index+=6;

				dh.Seek(index);
				dh.ReadLine(longname);
				break;
			}
			/* skip block */
			index+=blocklen;
		}
		dh.Close();
	}
}

#define FAT_SIZE 512
#define COMMONHEADERSIZE 21
#define ELEM_POINT	0x10
#define ELEM_IDXPNT	0x20
#define ELEM_POLYLINE	0x40
#define ELEM_POLYGON	0x80

MSGPXMap::MSGPXMap(const char *fn)
{
	int i;
	double c;
	long offset;
	long fatend;
	unsigned long filesize;
	char *fp;
	int copyrightoffset,copyrightsize,copyrightrecsize;
	MSFAT *labfat;
	MSFAT *rgnfat;
	MSFAT *trefat;

	m_numchildren=0;
	m_children=0;

	SetZoomLevels(MINMSZOOM,MAXMSZOOM);
	SetTileSize(256,256);

	for(i=0;i<MAXMSZOOM;++i)
	{
		c=(double)(1<<i);
		m_pixelsPerLonDegree[i]=c/360.0f;
  		m_negpixelsPerLonRadian[i] = -(c / (2.0f*PI));
		m_bitmapOrigo[i]=c/2.0f;
		m_numTiles[i] = 1<<i;
		SetSize(i,m_numTiles[i]*256,m_numTiles[i]*256);
	}

	m_filedata=0;
	fp=(char *)kGUI::LoadFile(fn,&filesize);
	if(!fp)
	{
		m_bad=true;
		return;	/* couldn't load file */
	}
	m_filedata=fp;

	/* if first char is non zero, then xor while file with value */
	if(fp[0])
	{
		char xorchar=fp[0];
		for(unsigned long ii=0;ii<filesize;++ii)
			fp[ii]^=xorchar;
	}

	/* is this a garmin file? */
	if(strncmp(fp+0x10,"DSKIMG",6))
	{
		m_bad=true;
		return;
	}
	if(strncmp(fp+0x41,"GARMIN",6))
	{
		m_bad=true;
		return;
	}

	/* parse the FAT area first */
	m_numfats=0;
	m_fats.Init(32,4);
	m_fats.SetGrow(true);
	
//	offset=ReadU8(fp+0x40)*512;
	offset=0x0600;
	fatend=ReadU32(fp+0x040c);
	m_blocksize= 1<<((int)ReadU8(fp + 0x61)+(int)ReadU8(fp + 0x62));

	/* skip empty fat blocks */
	while(ReadU8(fp+offset)==0)
	{
		offset+=FAT_SIZE;
	}

	while(ReadU8(fp+offset+1)==' ' && ReadU8(fp+offset+9)==' ')
	{
		offset+=FAT_SIZE;
	}

	while(1)
	{
		MSFAT *newfat;
		int part;
		int blockindex;
		unsigned short firstblock;
		unsigned short lastblock;

		if(fatend)
		{
			if(offset>=fatend)
				break;
		}
		else
		{
			if(ReadU8(fp+offset)!=1)
				break;
		}

		/* process the fat and add entries to the fat array */

		newfat=new MSFAT();
		newfat->filename.SetString(fp+offset+1,8);
		newfat->filetype.SetString(fp+offset+9,3);
		part=ReadU16(fp+offset+0x10);

		blockindex=0x20;
		firstblock=ReadU16(fp+offset+blockindex);
		do
		{
			blockindex+=2;
			if(blockindex==FAT_SIZE)
				break;
			if(ReadU16(fp+offset+blockindex)==0xffff)
				break;
		}while(1);
		lastblock=ReadU16(fp+offset+blockindex-2);

		if(!part)
		{
			newfat->size= ReadU32(fp+offset+0xC);
			newfat->blockstart= firstblock;
			newfat->blockend= lastblock;
			m_fats.SetEntry(m_numfats++,newfat);
		}
		else
		{
			MSFAT *addfat=LocateFile(newfat->filename.GetString(),newfat->filetype.GetString());
			addfat->blockend= lastblock;
			delete newfat;
		}
		offset+=FAT_SIZE;
	}

	/* make sure all 3 are there! */
	labfat=LocateType("LBL");
	rgnfat=LocateType("RGN");
	trefat=LocateType("TRE");

	if(!rgnfat || !labfat || !trefat)
	{
		m_bad=true;
		return;
	}

	/* where is the RGN area */
	{

		m_rgnoffset=(rgnfat->blockstart*m_blocksize);
		m_rgnsize=ReadU32(fp+m_rgnoffset+COMMONHEADERSIZE+4);
		/* FIXED: this was wrong in the sample code */
		m_rgnoffset+=ReadU16(fp+m_rgnoffset+0x15);	/* size of all header blocks, not just the first */
	}

	/* let's parse the TRE area */
	{
		int levelsize,subdivisionsize;
		int leveloffset,subdivisionoffset;

		offset=trefat->blockstart*m_blocksize;

		m_boundary.nw.dlat= Read24(fp+offset+COMMONHEADERSIZE + 0x0);	// north bound
		m_boundary.se.dlong= Read24(fp+offset+COMMONHEADERSIZE + 0x3);// east bound
		m_boundary.se.dlat= Read24(fp+offset+COMMONHEADERSIZE + 0x6);	// south bound
		m_boundary.nw.dlong= Read24(fp+offset+COMMONHEADERSIZE + 0x9);// west bound


//	printf("TRE offset= %x\n", ptr-pimg);
// Offset in TRE

		leveloffset= offset+ReadU32(fp+offset+COMMONHEADERSIZE + 0xC);	
		levelsize= ReadU32(fp+offset+COMMONHEADERSIZE + 0x10);
		subdivisionoffset= offset+ReadU32(fp+offset+COMMONHEADERSIZE + 0x14);
		subdivisionsize= (ReadU32(fp+offset+COMMONHEADERSIZE + 0x18))-4;

		copyrightoffset= offset+ReadU32(fp+offset+COMMONHEADERSIZE + (0x1c));
		copyrightsize= (ReadU32(fp+offset+COMMONHEADERSIZE + (0x20)));
		copyrightrecsize= (ReadU32(fp+offset+COMMONHEADERSIZE + (0x24)));

		m_numsubdivs=0;
		m_numlevels=levelsize>>2;
		m_levels.Alloc(m_numlevels);

		m_numdrawlevels=0;
		m_drawlevels.Alloc(10);
		m_drawlevels.SetGrow(true);

		/* 0=unlocked, 0x80=locked */
		if(!ReadU8(fp+offset+0x0d))
		{
			for (i=0;i<m_numlevels;++i)
			{
				unsigned char zoom;
				MSLEVEL *nl;
				
				nl=new MSLEVEL();
				zoom= ReadU8(fp+leveloffset);
				nl->zoom= zoom & 0xF;
				nl->inherit= (zoom & 0x80)>>7;
				nl->cbits= ReadU8(fp+leveloffset+1);	/* cbits = bits per coord */
				nl->nsubdivisions= ReadU16(fp+leveloffset+2);
				m_numsubdivs+=nl->nsubdivisions;	/* count total num of subdivs */
				m_levels.SetEntry(i,nl);
				leveloffset+=4;
			}

			m_subdivs.Alloc(m_numsubdivs);
			m_subdivs.SetGrow(true);
			m_numsubdivs=0;	/* used as index when filling array */

			for(i=m_numlevels-1;i>=0;--i)
			{
				MSLEVEL *level;
				int j;
				int nsubdivisions;
				int recsize= (i)?16:14;	/* level zero is smaller */

				level=m_levels.GetEntry((m_numlevels-1)-i);
				nsubdivisions = level->nsubdivisions;

				for (j= 0; j< nsubdivisions; ++j)
				{
					MSSUBDIV *sub;
					int shiftby= 24-level->cbits;

					sub=new MSSUBDIV();
					sub->level=i;
					LoadSub(sub,fp+subdivisionoffset,shiftby);
					sub->next_level_idx= (i) ? ReadU16(fp+subdivisionoffset+14):0;

					m_subdivs.SetEntry(m_numsubdivs++,sub);
					subdivisionoffset+= recsize;
				}
			}
		}
		else	/* level data is locked, must calc by scanning subdivs */
		{
			int subdivisionend=subdivisionoffset+subdivisionsize-4;
			int level=m_numlevels-1;
			int nums=0;
			int ncsub=1;
			int index;
			int nextlevelindex;
			bool smaller;

			{
				int mapw,maph;

				mapw=m_boundary.nw.dlat-m_boundary.se.dlat;
				maph=m_boundary.se.dlong-m_boundary.nw.dlong;
			//	DebugPrint("Map width=%d, Map Height=%d\n",mapw,maph);
			}

			for (i=0;i<m_numlevels;++i)
			{
				MSLEVEL *nl;
				
				nl=new MSLEVEL();
				nl->zoom=i;
				nl->inherit=0;
				nl->cbits=1;
				m_levels.SetEntry(i,nl);
			}

			m_subdivs.Alloc(200);
			m_subdivs.SetGrow(true);
			m_numsubdivs=0;	/* used as index when filling array */

			index=1;
			nextlevelindex=-1;
			nums=0;
			smaller=false;
			while(subdivisionoffset<subdivisionend)
			{
				MSSUBDIV *sub;

				if(level<0)
					break;

				sub=new MSSUBDIV();
				if(level)
				{
					if(ReadU16(fp+subdivisionoffset+14)>(subdivisionsize/14))
						smaller=true;
				}
				sub->level=level;
				if(LoadSub(sub,fp+subdivisionoffset,1)==false)
				{
					delete sub;
					m_bad=true;
					return;
				}
				else
				{
					++nums;
					if(level && smaller==false)
					{
						sub->next_level_idx=ReadU16(fp+subdivisionoffset+14);
						subdivisionoffset+=16;
					}
					else
					{
						sub->next_level_idx=0;
						subdivisionoffset+=14;
					}
					//DebugPrint("level=%d,Index=%d,w=%d,h=%d,Sub next=%d,droplevel=%d,last=%d\n",level,m_numsubdivs,sub->boundary.nw.dlat-sub->boundary.se.dlat,sub->boundary.se.dlong-sub->boundary.nw.dlong,sub->next_level_idx,nextlevelindex,sub->last);

					if(nextlevelindex==-1)
						nextlevelindex=sub->next_level_idx;

					m_levels.GetEntry((m_numlevels-1)-level)->nsubdivisions=nums;
					++index;
					if(sub->last)
					{
						--ncsub;
						if(!ncsub)
						{
							ncsub=nums;
							nums=0;
							--level;
						}
					}
					m_subdivs.SetEntry(m_numsubdivs++,sub);
				}
			}
			/* check to make sure cbits is correct */
			/* do this by calculating the bounds of all subdivs at each level */
			/* if they are larger than the map bounds then use previous size */
			{
				int mapw,maph;
				bool gotsub;
				int s;
				MSLEVEL *l;
				MSSUBDIV *sub;
				MSBOUND levelb;
				int levelw,levelh;
				int trylevel;
				double overw,overh,over;
				double lastover;
				double fullarea;
				double scalearea;

				mapw=m_boundary.nw.dlat-m_boundary.se.dlat;
				maph=m_boundary.se.dlong-m_boundary.nw.dlong;
				fullarea=fabs((double)mapw)*fabs((double)maph);

				for (i=0;i<m_numlevels;++i)
				{
					lastover=0.0f;
					l=m_levels.GetEntry(i);
//					trylevel=l->cbits;
					trylevel=0;
					while(1)
					{
						++trylevel;
						SetLevelShift(i,trylevel);
						gotsub=false;
						scalearea=0;
						for(s=0;s<m_numsubdivs;++s)
						{
							sub=m_subdivs.GetEntry(s);

							if(sub->level==i)
							{
								scalearea+=fabs((double)(sub->boundary.nw.dlat-sub->boundary.se.dlat))*fabs((double)(sub->boundary.se.dlong-sub->boundary.nw.dlong));
								if(gotsub==false)
								{
									gotsub=true;
									levelb=sub->boundary;
								}
								else
								{
									if(sub->boundary.nw.dlat>levelb.nw.dlat)
										levelb.nw.dlat=sub->boundary.nw.dlat;
									if(sub->boundary.nw.dlong<levelb.nw.dlong)
										levelb.nw.dlong=sub->boundary.nw.dlong;

									if(sub->boundary.se.dlat<levelb.se.dlat)
										levelb.se.dlat=sub->boundary.se.dlat;
									if(sub->boundary.se.dlong>levelb.se.dlong)
										levelb.se.dlong=sub->boundary.se.dlong;
								}
							}
						}
						/* now we have the max bounds for the level */
						/* compare to the map bounds */

						levelw=levelb.nw.dlat-levelb.se.dlat;
						levelh=levelb.se.dlong-levelb.nw.dlong;
						overw=(double)levelw/(double)mapw;
						overh=(double)levelh/(double)maph;
						if(overh>overw)
							over=overh;
						else
							over=overw;
						DebugPrint("fullarea=%f,scalearea=%f ( ratio =%f )\n",fullarea,scalearea,scalearea/fullarea);
						DebugPrint("level=%d,over=%f,overw=%f,overh=%f\n",trylevel,over,overw,overh);
//						if(scalearea>fullarea)
//							break;
						if(over>=1.1f)
							break;
						lastover=over;
//						if(overh>over)
//							over=overh;
//						if(overw>over)
//							over=overw;
					}
					/* pick the one closer to 1.0 */
//					if(scalearea>fullarea)
//						--trylevel;
					if((1.0f-lastover)<(over-1.0f))
						--trylevel;

					/* lets try this */
					DebugPrint("level used=%d\n\n",trylevel);

					SetLevelShift(i,trylevel);
				}
			}
		}

		/* calculate region ends by stopping at the next one and */
		/* for the last one, use the end of region area size */
		for(i=0;i<m_numsubdivs;++i)
		{
			MSSUBDIV *sub;
			long end;

			sub=m_subdivs.GetEntry(i);
			if(i==(m_numsubdivs-1))
				end=m_rgnsize;
			else
				end=m_subdivs.GetEntry(i+1)->rgn_offset;
			sub->rgn_end=end;
			CalcSubRegions(sub);
		}
	}

	/* calculate the level that actually have draw data */
	for(i=m_numlevels-1;i>=0;--i)
	{
		int j;
		MSLEVEL *l;
		MSSUBDIV *sub;
		bool drawlevel=false;
				
		l=m_levels.GetEntry(i);
		for(j=0;(j<m_numsubdivs && drawlevel==false);++j)
		{
			sub=m_subdivs.GetEntry(j);
			if(sub->level==i)
			{
				if(sub->elements&(ELEM_POINT+ELEM_IDXPNT+ELEM_POLYLINE+ELEM_POLYGON))
					drawlevel=true;
			}
		}
		l->drawlevel=drawlevel;
		if(drawlevel==true)
			m_drawlevels.SetEntry(m_numdrawlevels++,i);
	}

	/* get label area info */
	{
		offset=labfat->blockstart*m_blocksize;

		m_labelstart=(fp+offset+ReadU32(fp+offset+COMMONHEADERSIZE + 0x0));	
		m_labelsize= ReadU32(fp+offset+COMMONHEADERSIZE + 0x4);
		m_labelshift= ReadU8(fp+offset+COMMONHEADERSIZE + 0x8);
		assert(m_labelshift<4,"Error!");
		m_labelencoding= ReadU8(fp+offset+COMMONHEADERSIZE + 0x9);
		assert((m_labelencoding==6) || (m_labelencoding==9),"Unknown encoding format!");
	}

	{
		int c,loff;
		kGUIString *s;
		kGUIString cc;

		cc.Sprintf("%c ",169);	/* (c) */
		m_numcopyrightlines=copyrightsize/copyrightrecsize;
		m_copyrights.Alloc(m_numcopyrightlines);
		/* read the copyright strings? */
		for(c=0;c<m_numcopyrightlines;++c)
		{
			s=new kGUIString;
			m_copyrights.SetEntry(c,s);
			loff=Read24(fp+copyrightoffset+(copyrightrecsize*c));	/* label offset */
			ReadLabel(m_labelstart+loff,s);
			s->Insert(0,cc.GetString());
		}
	}

	/* get net area info, only found in autorouting maps */
	{
		MSFAT *netfat=LocateType("NET");
		if(netfat)
		{
			offset=netfat->blockstart*m_blocksize;

			m_netstart=(fp+offset+ReadU32(fp+offset+COMMONHEADERSIZE + 0x0));
			m_netsize= ReadU32(fp+offset+COMMONHEADERSIZE + 0x4);
			m_netshift= ReadU8(fp+offset+COMMONHEADERSIZE + 0x8);
			assert(m_netshift<4 && m_netshift>=0,"Error!");
		}
		else
		{
			m_netstart=0;
			m_netsize=0;
		}
	}

	/* is there a tdb file? */
	{

		kGUIString tdbfn;
	
		tdbfn.SetString(fn);
		tdbfn.Replace(".img",".tdb");

		fp=(char *)kGUI::LoadFile(tdbfn.GetString(),&filesize);
		if(fp)
		{
			const char *cfp;
			const char *efp=fp+filesize;
			int blocklen;
			unsigned char blockid;
			MSGPXChild *cc;

			/* pass 1, count number of children */
			cfp=fp;
			while(cfp<efp)
			{
				blockid=cfp[0];
				blocklen=ReadU16(cfp+1);
				cfp+=3;
				if(blockid==0x4c)
					++m_numchildren;
				cfp+=blocklen;
			}
			m_children=new MSGPXChild[m_numchildren];
			/* pass 2, fill them in */

			cfp=fp;
			cc=m_children;
			while(cfp<efp)
			{
				blockid=cfp[0];
				blocklen=ReadU16(cfp+1);
				cfp+=3;
				if(blockid==0x4c)
				{
//					cc->m_mapname.Sprintf("%08d.img",ReadU32(cfp+0));
					cc->m_mapnum=ReadU32(cfp+0);
					cc->m_nw.SetLat((double)Read32(cfp+8)*UNIT_TO_DEG32);
					cc->m_se.SetLon((double)Read32(cfp+12)*UNIT_TO_DEG32);
					cc->m_se.SetLat((double)Read32(cfp+16)*UNIT_TO_DEG32);
					cc->m_nw.SetLon((double)Read32(cfp+20)*UNIT_TO_DEG32);
					++cc;
				}
				cfp+=blocklen;
			}

			delete []fp;
			/* build a bsp for the children */
		}
	}
}

/* set the cbits shift value for this level and re-calc sub bounds */

void MSGPXMap::SetLevelShift(int l,int b)
{
	int s;
	MSLEVEL *lev;
	MSSUBDIV *sub;
	
	lev=m_levels.GetEntry(l);
	lev->cbits=b;

	for(s=0;s<m_numsubdivs;++s)
	{
		sub=m_subdivs.GetEntry(s);
		if(sub->level==l)
			ReLoadSub(sub,b);
	}
}

MSGPXMap::~MSGPXMap()
{
	int i;

	for(i=0;i<m_numcopyrightlines;++i)
		delete m_copyrights.GetEntry(i);
	for(i=0;i<m_numsubdivs;++i)
		delete m_subdivs.GetEntry(i);
	for(i=0;i<m_numlevels;++i)
		delete m_levels.GetEntry(i);
	for(i=0;i<m_numfats;++i)
		delete m_fats.GetEntry(i);

	if(m_children)
		delete []m_children;

	if(m_filedata)
		delete []m_filedata;
}

void MSGPXMap::ToMap(GPXCoord *c,int *sx,int *sy)
{
	double e;
	int z=GetZoom();

  	sx[0] = (int)(floor((m_bitmapOrigo[z] + c->GetLon() * m_pixelsPerLonDegree[z])*256.0f));
  	e = sin(c->GetLat() * (PI/180.0f));

  	if(e > 0.9999)
    	e = 0.9999;

  	if(e < -0.9999)
    	e = -0.9999;

  	sy[0] = (int)(floor((m_bitmapOrigo[z] + 0.5f * log((1.0f + e) / (1.0f - e)) * m_negpixelsPerLonRadian[z])*256.0f));
}

/* convert to map using doubles */
void MSGPXMap::ToMap(GPXCoord *c,double *sx,double *sy)
{
	double e;
	int z=GetZoom();

  	sx[0] = ((m_bitmapOrigo[z] + c->GetLon() * m_pixelsPerLonDegree[z])*256.0f);
  	e = sin(c->GetLat() * (PI/180.0f));

  	if(e > 0.9999)
    	e = 0.9999;

  	if(e < -0.9999)
    	e = -0.9999;

  	sy[0] = ((m_bitmapOrigo[z] + 0.5f * log((1.0f + e) / (1.0f - e)) * m_negpixelsPerLonRadian[z])*256.0f);
}

/* convert from screen+scroll values to lon/lat */
void MSGPXMap::FromMap(int sx,int sy,GPXCoord *c)
{
	double e;
	int z=GetZoom();

	c->SetLon(((double)sx - (m_bitmapOrigo[z]*256.0f)) / (m_pixelsPerLonDegree[z]*256.0f));
	e = ((double)sy - (m_bitmapOrigo[z]*256.0f)) / (m_negpixelsPerLonRadian[z]*256.0f);
	c->SetLat((2.0f * atan(exp(e)) - PI / 2.0f) / (PI/180.0f));
}

static kGUIColor polycolours[]={
	DrawColor(0,0,255),			//0 unknown [lake]
	DrawColor(215,215,245),		//1 town_poly, City (>200k)
	DrawColor(220,220,250),		//2 town_poly, City (<200k)
	DrawColor(235,230,220),		//3 !city
	DrawColor(230,230,255),		//4 !small city
	DrawColor(180,180,180),		//5 parking lot
	DrawColor(160,160,160),		//6 parking garage
	DrawColor(255,255,192),		//7 !airport (yellow)
	DrawColor(209,208,205),		//8  shopping center
	DrawColor(128,128,255),		//9 marina
	DrawColor(192,192,192),		//10 !college or university
	DrawColor(229,198,195),		//11 hospital
	DrawColor(192,192,192),		//12 industrial
	DrawColor(255,225,225),		//13 !reservation (brown)
	DrawColor(192,192,192),	//14 airport runway
	DrawColor(0,0,0),		//15 
	DrawColor(0,0,0),		//16 
	DrawColor(0,0,0),		//17 
	DrawColor(0,0,0),		//18 
	DrawColor(192,192,192),		//19 !man made area, hi rise buildings
	DrawColor(167,204,149),	//20 national park
	DrawColor(167,204,149),	//21 national park
	DrawColor(167,204,149),	//22 national park
	DrawColor(167,204,149),	//23 city park
	DrawColor(167,214,149),	//24 golf
	DrawColor(225,255,225),		//25 !sport park (light green)
	DrawColor(175,175,175),		//26 !cemetery
	DrawColor(0,0,0),			//27 ferry route
	DrawColor(0,0,0),			//28 
	DrawColor(0,0,0),			//29 
	DrawColor(167,204,149),		//30 state park
	DrawColor(167,204,149),		//31 state park
	DrawColor(167,204,149),		//32 park
	DrawColor(0,0,0),		//33 
	DrawColor(0,0,0),		//34 
	DrawColor(0,0,0),		//35 
	DrawColor(0,0,0),		//36 
	DrawColor(0,0,0),		//37 
	DrawColor(0,0,0),		//38 
	DrawColor(0,0,0),		//39 
	DrawColor(153,179,224),	//40 ocean 
	DrawColor(153,179,214),	//41 water reservoir
	DrawColor(0,0,0),		//42 
	DrawColor(0,0,0),		//43 
	DrawColor(0,0,0),		//44 
	DrawColor(0,0,0),		//45 
	DrawColor(0,0,0),		//46 
	DrawColor(0,0,0),		//47 
	DrawColor(0,0,0),		//48 
	DrawColor(0,0,0),		//49 
	DrawColor(64,64,255),	//50 sea 
	DrawColor(0,0,0),		//51 
	DrawColor(0,0,0),		//52 
	DrawColor(0,0,0),		//53 
	DrawColor(0,0,0),		//54 
	DrawColor(0,0,0),		//55 
	DrawColor(0,0,0),		//56 
	DrawColor(0,0,0),		//57 
	DrawColor(0,0,0),		//58 
	DrawColor(167,204,149),	//59 unknown
	DrawColor(153,179,204),	//60 lake
	DrawColor(153,179,204),	//61 lake
	DrawColor(153,179,204),	//62 lake
	DrawColor(153,179,204),	//63 lake
	DrawColor(153,179,204),	//64 lake / inlet
	DrawColor(153,179,204),	//65 lake
	DrawColor(153,179,204),	//66 lake
	DrawColor(153,179,204),	//67 lake
	DrawColor(153,179,204),	//68 lake
	DrawColor(0,0,0),		//69 unknown
	DrawColor(153,179,214),	//70 river
	DrawColor(153,179,214),	//71 river
	DrawColor(153,179,214),	//72 river / inlet
	DrawColor(153,179,214),	//73 river
	DrawColor(0,0,0),		//74 background do not draw
	DrawColor(0,0,0),		//75 background do not draw
	DrawColor(153,179,214),	//76 intermittent river/lake
	DrawColor(128,255,192),	//77 glacier
	DrawColor(32,255,32),	//78 orchard or plantation
	DrawColor(167,204,149),	//79 scrub
	DrawColor(167,204,149),	//80 woods
	DrawColor(167,204,149),	//81 wetland
	DrawColor(167,204,149),	//82 tundra
	DrawColor(163,184,209)};	//83 flats / shoreline




enum
{
PL_NORMAL,
PL_TRAINTRACKS
};

typedef struct
{
	int thickindex;
	int drawlabel;
	int type;
	kGUIColor colour;
}POLYLINEINFO_DEF;

/* thickness for roads */

static double thickinfo[ROADGROUP_NUM][MAXMSZOOM]={
	//0    1    2    3    4    5    6    7    8    0   10    11    12    13    14    15    16    17    18    19
	{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},		/* line ( topo etc. )*/
	{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f, 1.0f, 1.0f, 1.0f, 2.5f, 5.0f, 7.0f, 8.0f, 9.0f,10.0f},		/* creek */
	{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.4f,0.5f, 0.6f, 0.7f, 1.0f, 2.0f, 5.0f, 8.0f, 7.0f,10.0f,11.0f},		/* narrow street */
	{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,1.0f, 1.0f, 1.0f, 2.0f, 4.5f, 9.0f,12.0f,13.0f,14.0f,15.0f},		/* side street */
	{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,2.0f,2.0f, 2.5f, 3.0f, 4.0f, 6.0f,11.0f,13.0f,14.0f,15.0f,16.0f},		/* ramps */
	{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,2.0f,2.5f,2.0f, 4.0f, 6.0f, 9.0f,10.0f,13.0f,14.0f,15.0f,16.0f,17.0f},		/* artery */
	{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,2.0f,3.0f,4.0f,5.0f, 7.0f, 9.0f,12.0f,15.0f,16.0f,17.0f,18.0f,19.0f,20.0f}};	/* highway */

static POLYLINEINFO_DEF polylineinfo[]={
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,0,255)},	//0 
	{ROADGROUP_HIGHWAY,1,PL_NORMAL,DrawColor(242,191,36)},	//1 highway
	{ROADGROUP_HIGHWAY,1,PL_NORMAL,DrawColor(242,181,36)},	//2 highway
	{ROADGROUP_COLLECTOR,1,PL_NORMAL,DrawColor(255,250,112)}, //3 artiery
	{ROADGROUP_COLLECTOR,1,PL_NORMAL,DrawColor(255,250,112)},	//4 artiery
	{ROADGROUP_COLLECTOR,1,PL_NORMAL,DrawColor(255,250,112)},	//5 artiery
	{ROADGROUP_STREET,1,PL_NORMAL,DrawColor(255,255,255)},	//6 street
	{ROADGROUP_STREET,1,PL_NORMAL,DrawColor(255,255,255)},		//7 
	{ROADGROUP_STREET,1,PL_NORMAL,DrawColor(255,255,255)},		//8 
	{ROADGROUP_RAMPS,1,PL_NORMAL,DrawColor(249,222,77)},	//9 on ramp
	{ROADGROUP_STREET,1,PL_NORMAL,DrawColor(255,255,255)},		//10 rural street
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//11
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//12
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//13
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(192,192,192)},//14 
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//15 
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//16 
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//17 
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//18 
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//19 
	{ROADGROUP_LINE,1,PL_TRAINTRACKS,DrawColor(128,255,128)},	//20 train tracks
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(128,255,128)},	//21 
	{ROADGROUP_NARROWSTREET,1,PL_NORMAL,DrawColor(255,255,255)},		//22 narrow street 
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(64,255,64)},	//23 
	{ROADGROUP_CREEK,1,PL_NORMAL,DrawColor(0,0,255)},		//24  creek
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//25  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//26  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(255,255,255)},	//27 ferry route  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//28  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//29  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//30  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(128,128,128)},	//31  topo line
	{ROADGROUP_LINE,0,PL_NORMAL,DrawColor(134,134,134)},	//32  topo line
	{ROADGROUP_LINE,0,PL_NORMAL,DrawColor(140,140,140)},	//33  topo line
	{ROADGROUP_LINE,0,PL_NORMAL,DrawColor(146,146,146)},	//34  topo line
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//35  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//36  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//37  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//38  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//39  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(0,255,0)},		//40  
	{ROADGROUP_LINE,1,PL_NORMAL,DrawColor(255,0,0)}};	//41 power line  

	static int mlevels[MAXMSZOOM]={
		0,0,0,0,1,
		1,2,2,3,3,
		4,4,4,5,5,
		5,5,5,5,5};

int MSGPXMap::DrawTile(int tx,int ty)
{
	unsigned int i;
	MSSUBDIV *sub;
	MSGPXMap *map;

	if(m_bad)
		return(TILE_ERROR);

	m_lc.Clear();

	m_tx=tx<<8;
	m_ty=ty<<8;
	/* expand so lines right on the edge are grabbed too */
	m_tilecorners.lx=(tx<<8)-CLIPEXPAND;		/* tiles are 256x256 */
	m_tilecorners.rx=((tx+1)<<8)+CLIPEXPAND;
	m_tilecorners.ty=(ty<<8)-CLIPEXPAND;		/* tiles are 256x256 */
	m_tilecorners.by=((ty+1)<<8)+CLIPEXPAND;

	/* keep track of drawn icons so we don't draw them too close together */
	m_numiconsdrawn=0;

	m_numdrawsubs=0;
	kGUI::DrawRect(0,0,256,256,DrawColor(242,239,233));

	/* add all sub areas to list */
	DrawTile(mlevels[GetZoom()]);

	/* add all polys that overlap the tile to the draw list */
	m_numsortpolys=0;
	m_sortpolysheap.Reset();

	for(i=0;i<m_numdrawsubs;++i)
	{
		sub=m_drawsubdivs.GetEntry(i);
		map=m_drawmaps.GetEntry(i);

		//debug draw
#if 0
		{
			GPXCoord nw;
			GPXCoord se;
			kGUIColor c;
			kGUICorners subcorners;

			c=DrawColor(255,i&255,i&255);
			nw.SetLat(ToDegrees(sub->boundary.nw.dlat));
			nw.SetLon(ToDegrees(sub->boundary.nw.dlong));
			se.SetLat(ToDegrees(sub->boundary.se.dlat));
			se.SetLon(ToDegrees(sub->boundary.se.dlong));
			ToMap(&nw,&subcorners.lx,&subcorners.ty);
			ToMap(&se,&subcorners.rx,&subcorners.by);

			kGUI::DrawRect(subcorners.lx-m_tx,subcorners.ty-m_ty,subcorners.rx-m_tx,subcorners.by-m_ty,c);
		}
#endif
		map->AddSubPolys(sub);
	}

	if(m_numsortpolys>1)
		m_sortpolys.Sort(m_numsortpolys,SortPolygonsType);

	/* draw all polygons in sorted order */
	for(i=0;i<m_numsortpolys;++i)
	{
		POLYSORT_DEF ps;

		ps=m_sortpolys.GetEntry(i);
		ps.map->DrawPoly(&ps);
	}

	/* reset roadgroups to empty */
	for(i=0;i<ROADGROUP_NUM;++i)
		m_roadgroupspolys[i]=0;

	/* now draw all labels and lines */
	for(i=0;i<m_numdrawsubs;++i)
	{
		sub=m_drawsubdivs.GetEntry(i);
		map=m_drawmaps.GetEntry(i);
		map->DrawSub(sub);
	}

	/* draw roadgroups roads */
	for(i=0;i<ROADGROUP_NUM;++i)
	{
		for(unsigned int j=0;j<m_roadgroupspolys[i];++j)
		{
			POLYSORT_DEF ps;

			ps=m_roadgroups[i].GetEntry(j);
			kGUI::DrawFatPolyLine(3,ps.numpoints,ps.points,polylineinfo[ps.polytype].colour,(float)ps.thickness);
		}
	}

	/* draw all polygon labels in sorted order */

	if(m_numsortpolys>1)
		m_sortpolys.Sort(m_numsortpolys,SortPolygonsLeft);

	for(i=0;i<m_numsortpolys;++i)
	{
		POLYSORT_DEF ps;

		ps=m_sortpolys.GetEntry(i);
		if(ps.numlabels)
			ps.map->DrawPolyLabel(&ps);
	}

	/* draw roadgroups labels */
	for(i=0;i<ROADGROUP_NUM;++i)
	{
		for(unsigned int j=0;j<m_roadgroupspolys[i];++j)
		{
			POLYSORT_DEF ps;

			ps=m_roadgroups[i].GetEntry(j);
			ps.map->DrawRoadGroupLabels(&ps);
		}
	}

	return(TILE_OK);
}

bool MSGPXMap::DrawTile(int level)
{
	int i;
	MSSUBDIV *sub;
	kGUICorners subcorners;
	GPXCoord nw;
	GPXCoord se;

	if(m_bad)
		return(false);

	/* go to a child level? */
	if( m_numchildren && level>m_numdrawlevels)
	{
		int c;
		MSGPXChild *cc;

		cc=m_children;
		for(c=0;c<m_numchildren;++c)
		{
			ToMap(&cc->m_nw,&subcorners.lx,&subcorners.ty);
			ToMap(&cc->m_se,&subcorners.rx,&subcorners.by);
			
			if(kGUI::Overlap(&m_tilecorners,&subcorners))
			{
				/* load img file, draw it */
				if(!cc->m_map)
				{
					const char *fn;

					fn=MSGPXFName::GetFName(cc->m_mapnum);
					if(fn)
						cc->m_map=new MSGPXMap(fn);
				}

				if(cc->m_map)
				{
					cc->m_map->SetZoom(GetZoom());
					cc->m_map->DrawTile(level-m_numdrawlevels);
				}
			}
			++cc;
		}
	}
	else
	{
		int maplevel;

		if(level>=m_numdrawlevels)
			level=m_numdrawlevels-1;
		maplevel=m_drawlevels.GetEntry(level);
		
		for(i=0;i<m_numsubdivs;++i)
		{
			sub=m_subdivs.GetEntry(i);
			if(sub->level==maplevel)
			{
				nw.SetLat(ToDegrees(sub->boundary.nw.dlat));
				nw.SetLon(ToDegrees(sub->boundary.nw.dlong));
				se.SetLat(ToDegrees(sub->boundary.se.dlat));
				se.SetLon(ToDegrees(sub->boundary.se.dlong));
				ToMap(&nw,&subcorners.lx,&subcorners.ty);
				ToMap(&se,&subcorners.rx,&subcorners.by);

				if(kGUI::Overlap(&m_tilecorners,&subcorners))
				{
					m_drawmaps.SetEntry(m_numdrawsubs,this);
					m_drawsubdivs.SetEntry(m_numdrawsubs++,sub);
				}
			}
		}
	}
	return(true);
}

/*****************************************************************************/

MSFAT *MSGPXMap::LocateFile(const char *name,const char *type)
{
	int i;
	MSFAT *entry;

	for(i=0;i<m_numfats;++i)
	{
		entry=m_fats.GetEntry(i);
		if(!strcmp(entry->filename.GetString(),name) && !strcmp(entry->filetype.GetString(),type))
			return(entry);
	}
	assert(false,"file not found!");
	return(0);
}

/* return null if type not found */
MSFAT *MSGPXMap::LocateType(const char *type)
{
	int i;
	MSFAT *entry;

	for(i=0;i<m_numfats;++i)
	{
		entry=m_fats.GetEntry(i);
		if(!strcmp(entry->filetype.GetString(),type))
			return(entry);
	}
	return(0);
}

unsigned char MSGPXMap::ReadU8(const char *fp)
{
	unsigned char v1=fp[0]&255;
	return(v1);
}

unsigned short MSGPXMap::ReadU16(const char *fp)
{
	unsigned short v1=fp[0]&255;
	unsigned short v2=fp[1]&255;
	return(v1|(v2<<8));
}

short MSGPXMap::Read16(const char *fp)
{
	short val16=ReadU16(fp);
	//if ( val16 > 0x7FFF )
	//	val16-=0xFFFF;
	return(val16);
}

unsigned int MSGPXMap::ReadU24(const char *fp)
{
	unsigned int v1=fp[0]&255;
	unsigned int v2=fp[1]&255;
	unsigned int v3=fp[2]&255;
	return(v1|(v2<<8)|(v3<<16));
}

int MSGPXMap::Read24(const char *fp)
{
	int val24=ReadU24(fp);
	if ( val24 > 0x7FFFFF )
		val24-=0xFFFFFF;
	return(val24);
}

unsigned int MSGPXMap::ReadU32(const char *fp)
{
	unsigned int v1=fp[0]&255;
	unsigned int v2=fp[1]&255;
	unsigned int v3=fp[2]&255;
	unsigned int v4=fp[3]&255;
	return(v1|(v2<<8)|(v3<<16)|(v4<<24));
}

int MSGPXMap::Read32(const char *fp)
{
	int val32=ReadU32(fp);
	if ( val32 > 0x7FffFFFF )
		val32-=0xFFFFFFFF;
	return(val32);
}

void MSGPXMap::ReLoadSub(MSSUBDIV *sub,int shiftby)
{
	LoadSub(sub,sub->from,shiftby);
}

bool MSGPXMap::LoadSub(MSSUBDIV *sub,const char *fp,int shiftby)
{
	unsigned int width, height;
	unsigned int rgninfo;

	sub->from=fp;
	sub->shiftby= shiftby;
	sub->rgn_end= -1;
	
	rgninfo= ReadU32(fp);
	sub->rgn_offset= (rgninfo&0xFFFFFF);
#if 1
	if(sub->rgn_offset>m_rgnsize)
		return(false);
#else
	assert(sub->rgn_offset<m_rgnsize,"Offset into region too large!");
#endif

	sub->elements= rgninfo>>24;
	sub->center.dlong= Read24(fp+4);
	sub->center.dlat= Read24(fp+7);
	width= ReadU16(fp+10);
	sub->last= (width & 0x8000) ? 1:0;
	width= (width & 0x7fff)<<shiftby;
	height= (Read16(fp+12))<<shiftby;

	sub->boundary.nw.dlat= sub->center.dlat+height;
	sub->boundary.nw.dlong= sub->center.dlong-width;
	sub->boundary.se.dlat= sub->center.dlat-height;
	sub->boundary.se.dlong= sub->center.dlong+width;
	return(true);
}

/* this needs to be done after all the regions have been parsed */
/* as we need to know then end region offset */

void MSGPXMap::CalcSubRegions(MSSUBDIV *sub)
{
	int oi;
	int roff;
	int numoffs;
	int ri;
	const char *rgnoffsets[5];

	/* since we have the region offset and types we can get offsets to all 4 region types */

	/* the region offset points to the region data for this sundivision */
	/* since there can be 4 different types of region data, there can be */
	/* 0-3 offsets here first, the first one is assumed to start after */
	/* the offsets to the others */
	
	if(!(sub->elements&(ELEM_POINT+ELEM_IDXPNT+ELEM_POLYLINE+ELEM_POLYGON)))	/* are there any elements for this subdivision? */
		return;			/* nope, quit */

	roff=m_rgnoffset+sub->rgn_offset;
	numoffs=-1;
	if(sub->elements&ELEM_POINT)
		++numoffs;
	if(sub->elements&ELEM_IDXPNT)
		++numoffs;
	if(sub->elements&ELEM_POLYLINE)
		++numoffs;
	if(sub->elements&ELEM_POLYGON)
		++numoffs;
	
	/* we insert at the beginning a first item offset so we don't */
	/* have to have any special code for handling it differently */

	/* then we have 0-3 offsets followed by an END offset */
	/* the length of each section then is the difference between */
	/* it's offset and the next one in the list */

	rgnoffsets[0]=m_filedata+roff+(numoffs<<1);	/* first one */
	for(oi=0;oi<numoffs;++oi)
		rgnoffsets[oi+1]=m_filedata+roff+(ReadU16(m_filedata+roff+(oi<<1)));

	rgnoffsets[numoffs+1]=m_filedata+m_rgnoffset+sub->rgn_end;
	assert(rgnoffsets[numoffs+1]>rgnoffsets[numoffs],"Internal Error!");

	ri=0;
	if(sub->elements&ELEM_POINT)
	{
		sub->m_pntstart=rgnoffsets[ri];
		sub->m_pntend=rgnoffsets[ri+1];
		++ri;
	}
	if(sub->elements&ELEM_IDXPNT)
	{
		sub->m_idxstart=rgnoffsets[ri];
		sub->m_idxend=rgnoffsets[ri+1];
		++ri;
	}
	if(sub->elements&ELEM_POLYLINE)
	{
		sub->m_linestart=rgnoffsets[ri];
		sub->m_lineend=rgnoffsets[ri+1];
		++ri;
	}
	if(sub->elements&ELEM_POLYGON)
	{
		sub->m_polystart=rgnoffsets[ri];
		sub->m_polyend=rgnoffsets[ri+1];
	}
}

static unsigned int extendbits[32+1]={
	0xffffffff,
	0xfffffffe,
	0xfffffffc,
	0xfffffff8,
	0xfffffff0,
	0xffffffe0,
	0xffffffc0,
	0xffffff80,

	0xffffff00,
	0xfffffe00,
	0xfffffc00,
	0xfffff800,
	0xfffff000,
	0xffffe000,
	0xffffc000,
	0xffff8000,

	0xffff0000,
	0xfffe0000,
	0xfffc0000,
	0xfff80000,
	0xfff00000,
	0xffe00000,
	0xffc00000,
	0xff800000,

	0xff000000,
	0xfe000000,
	0xfc000000,
	0xf8000000,
	0xf0000000,
	0xe0000000,
	0xc0000000,
	0x80000000,

	0x00000000};	/* no bits to extend */
	
int MSGPXMap::GetPoint(kGUIBitStream *bs,int nbits,int sign)
{
	int val;
	int signbit;
	int longval;

	val= bs->ReadU(nbits);
	if ( sign > 0 )
		return(val);
	if(sign<0)
		return(-val);
	signbit= 1<<(nbits-1);
	if (val==signbit)
	{
		longval=0;
		while(1)
		{
			longval+=(signbit-1);
			val=bs->ReadU(nbits);
			if(val!=signbit)
				break;
		}
		if ( val&signbit )
		{
			val|=extendbits[nbits];
			val-=longval;
			return(val);
		}
		return(val+longval);
	}
	if ( val&signbit )
		val|=extendbits[nbits];

	return(val);
}

/**********************************************************************/

int MSGPXMap::SortPolygonsType(const void *v1,const void *v2)
{
	POLYSORT_DEF *i1;
	POLYSORT_DEF *i2;

	i1=(POLYSORT_DEF *)v1;
	i2=(POLYSORT_DEF *)v2;

	return(i1->polytype-i2->polytype);
}

int MSGPXMap::SortPolygonsLeft(const void *v1,const void *v2)
{
	POLYSORT_DEF *i1;
	POLYSORT_DEF *i2;

	i1=(POLYSORT_DEF *)v1;
	i2=(POLYSORT_DEF *)v2;

	return(i1->corners.lx-i2->corners.lx);
}

void MSGPXMap::AddSubPolys(MSSUBDIV *sub)
{
	const char *rstart;
	const char *rend;
	POLYSORT_DEF ps;

	/* save polygon */
	if(sub->elements&ELEM_POLYGON)
	{
		rstart=sub->m_polystart;
		rend=sub->m_polyend;
		while(rstart<rend && rstart)
		{
			ps.rstart=rstart;
			rstart=ReadPoly(sub,rstart,ELEM_POLYGON);
			if(rstart && kGUI::Overlap(&m_tilecorners,&m_polycorners))
			{
				/* draw 3 first, then everything in order after that? */
//				if(m_polytype<3)
//					m_polytype=4;

				ps.map=this;
				ps.polytype=m_polytype;
				ps.corners=m_polycorners;

				/* copy decompressed points to a temporary heap so we don't need to */
				/* decompress them again, this is a speed optimization! */

				ps.numpoints=m_numpoints;
				ps.points=(kGUIFPoint2 *)m_sortpolysheap.Alloc(m_numpoints*sizeof(kGUIFPoint2));
				memcpy(ps.points,m_ppoints,m_numpoints*sizeof(kGUIFPoint2));

				/* save labels associated with this poly */
				ps.numlabels=m_numlabels;
				memcpy(ps.curlabels,m_curlabels,sizeof(m_curlabels));

				m_sortpolys.SetEntry(m_numsortpolys++,ps);
			}
		}
	}
}

void MSGPXMap::DrawPoly(POLYSORT_DEF *ps)
{
	int l;
	kGUIColor c;

	l=ps->polytype;
	if((l==74) || (l==75))
		return;	/* background, no need to draw */

	if(ps->polytype<(sizeof(polycolours)/sizeof(kGUIColor)))
		c=polycolours[ps->polytype];
	else
		c=DrawColor(0,0,0);

	if(c)
		kGUI::DrawPoly(ps->numpoints,ps->points,c);
	else
	{
		int aw=ps->corners.rx-ps->corners.lx;
		int ah=ps->corners.by-ps->corners.ty;
		int lx,ly,lw,lh;

		/* since no color is defined, just draw an outline */
		kGUI::DrawPolyLine(ps->numpoints,ps->points,DrawColor(32,32,32));
		kGUI::DrawLine((int)ps->points[0].x,(int)ps->points[0].y,(int)ps->points[ps->numpoints-1].x,(int)ps->points[ps->numpoints-1].y,DrawColor(32,32,32));

		/* then draw the color number in the center so we can then define a color for this type */

		m_t[0].SetFontSize(12);
		m_t[0].Sprintf("%d",ps->polytype);
		lw=m_t[0].GetWidth();
		lh=m_t[0].GetLineHeight();
		lx=(ps->corners.lx-m_tx)+(aw>>1)-(lw>>1);
		ly=(ps->corners.ty-m_ty)+(ah>>1);
		DrawLabel(&m_t[0],lx,ly,lw,lh,0.0f,false);
	}
}

void MSGPXMap::DrawPolyLabel(POLYSORT_DEF *ps)
{
	int l;
	//double ah=ps->corners.by-ps->corners.ty; /* height of poly area */
	double aw=ps->corners.rx-ps->corners.lx; /* width of poly area */
	double lgh;	/* label group height */
	double cx,y,lw,lh;
	int fs;
	int validlabels;

	/* unpack labels */
	validlabels=0;
	for(l=0;l<ps->numlabels;++l)
	{
		ReadLabel(ps->curlabels[l],&m_t[validlabels]);
		if(m_t[validlabels].GetLen())
			++validlabels;
	}

	if(!validlabels)
		return;

	fs=7;
	/* expand font size until it doesn't fit anymore or 20 point */
	do
	{
		for(l=0;l<validlabels;++l)
		{
			m_t[l].SetFontSize(fs);
			if(m_t[l].GetWidth()>aw)
				goto hit;
		}
		++fs;
	}while(fs<14);
hit:;
	if(fs==7)
		return;

	for(l=0;l<validlabels;++l)
		m_t[l].SetFontSize(fs);

	lh=m_t[0].GetLineHeight();		/* height of 1 line of text */
	lgh=lh*validlabels;		/* height of all labels on this polygon */

#if 0
	kGUI::DrawLine(
		ps->corners.lx-m_tx,
		ps->corners.ty-m_ty,
		ps->corners.rx-m_tx,
		ps->corners.by-m_ty,
		DrawColor(0,0,0));

	kGUI::DrawLine(
		ps->corners.rx-m_tx,
		ps->corners.ty-m_ty,
		ps->corners.lx-m_tx,
		ps->corners.by-m_ty,
		DrawColor(0,0,0));
#endif

	cx=((ps->corners.lx+ps->corners.rx)/2.0f)-m_tx;
	y=((ps->corners.by+ps->corners.ty-lgh)/2.0f)-m_ty;

	/* draw em */
	for(l=0;l<validlabels;++l)
	{
		lw=m_t[l].GetWidth();
		DrawLabel(&m_t[l],cx-(lw/2.0f),y,lw,lh,0.0f,false);
		y+=lh;
	}
	
}

void MSGPXMap::DrawRoadGroupLabels(POLYSORT_DEF *ps)
{
	int l,fs;
	double roadthickness;

	for(l=0;l<ps->numlabels;++l)
	{
		ReadLabel(ps->curlabels[l],&m_t[l]);
#if 0
		//debug, append poly type
		m_t.ASprintf("<%d>",m_polytype);
#endif
		if(m_t[l].GetLen())
		{
			roadthickness=ps->thickness;
			fs=(int)(roadthickness*2.2f);
			if(fs>4)
			{
				m_pixlen=ps->pixlen;
				m_t[l].SetFontSize(fs);
				DrawLineLabel(&m_t[l],ps->numpoints,ps->points,-roadthickness*0.9f,true);
			}
		}
	}
}

void MSGPXMap::DrawSub(MSSUBDIV *sub)
{
	const char *rstart;
	const char *rend;
	int l;
	POLYSORT_DEF ps;
	bool skiplabel;
	double edge;

	/* no elements to draw, abort */
	if(!sub->elements)
		return;

	/* draw polylines */

	if(sub->elements&ELEM_POLYLINE)
	{
		rstart=sub->m_linestart;
		rend=sub->m_lineend;
		while(rstart<rend && rstart)
		{
			POLYLINEINFO_DEF *pi;
			double thickness;

			rstart=ReadPoly(sub,rstart,ELEM_POLYLINE);
			if(rstart && kGUI::Overlap(&m_tilecorners,&m_polycorners))
			{
				skiplabel=false;
				if(m_polytype<(sizeof(polylineinfo)/sizeof(POLYLINEINFO_DEF)))
					pi=polylineinfo+m_polytype;
				else
					pi=polylineinfo;	/* outside of table, use entry 0 */

				switch(pi->type)
				{
				case PL_NORMAL:
					thickness=thickinfo[pi->thickindex][GetZoom()];
					if(thickness==1.0f)
						kGUI::DrawPolyLine(m_numpoints,m_ppoints,pi->colour);
					else if(thickness>1.0f)
					{
						thickness=thickness*0.5f;
						ps.map=this;
						ps.polytype=m_polytype;
						ps.corners=m_polycorners;
						ps.thickness=thickness;
						ps.pixlen=m_pixlen;

						/* copy decompressed points to a temporary heap so we don't need to */
						/* decompress them again, this is a speed optimization! */

						ps.numpoints=m_numpoints;
						ps.points=(kGUIFPoint2 *)m_sortpolysheap.Alloc(m_numpoints*sizeof(kGUIFPoint2));
						memcpy(ps.points,m_ppoints,m_numpoints*sizeof(kGUIFPoint2));

						/* save labels associated with this polyline */
						if(GetZoom()>=13 && m_numlabels && pi->drawlabel)
						{
							skiplabel=true;
							ps.numlabels=m_numlabels;
							memcpy(ps.curlabels,m_curlabels,sizeof(m_curlabels));
						}
						else
							ps.numlabels=0;
						m_roadgroups[pi->thickindex].SetEntry(m_roadgroupspolys[pi->thickindex]++,ps);
						edge=MIN(MAX(0.75f,thickness*0.115f),1.5f);
						kGUI::DrawFatPolyLine(3,m_numpoints,m_ppoints,DrawColor(128,128,128),(float)(thickness+edge),0.66f);

					}
				break;
				case PL_TRAINTRACKS:
					if(GetZoom()>13)
						DrawTrainTracks(m_numpoints,m_ppoints,DrawColor(64,64,64));
					else
						kGUI::DrawPolyLine(m_numpoints,m_ppoints,DrawColor(64,64,64));
				break;
				}

				if(GetZoom()>=13 && m_numlabels && pi->drawlabel && skiplabel==false)
				{
					for(l=0;l<m_numlabels;++l)
					{
						ReadLabel(m_curlabels[l],&m_t[l]);
#if 0
						m_t.ASprintf("<%d>",m_polytype);
#endif
						if(m_t[l].GetLen())
						{
							if(m_labelicon>=0)
								m_t[l].SetFontSize(11);
							else
							{
								switch(GetZoom())
								{
								case 13:
								case 14:
									m_t[l].SetFontSize(8);
								break;
								case 15:
									m_t[l].SetFontSize(9);
								break;
								case 16:
									m_t[l].SetFontSize(10);
								break;
								default:
									m_t[l].SetFontSize(11);
								break;
								}
							}
							DrawLineLabel(&m_t[l],m_numpoints,m_ppoints,5,true);
						}
					}
				}
			}
		}
	}

	if(sub->elements&ELEM_POINT)
	{
		rstart=sub->m_pntstart;
		rend=sub->m_pntend;
		while(rstart<rend)
		{
			rstart=ReadPoint(sub,rstart);
			kGUI::DrawRect((int)m_ppoints[0].x,(int)m_ppoints[0].y,(int)m_ppoints[0].x+2,(int)m_ppoints[0].y+2,DrawColor(0,64,192));

			if(m_numlabels && GetZoom()>13)
			{
				//save text to be drawn after roads etc.
				ps.map=this;
				ps.polytype=m_polytype;
				ps.corners=m_polycorners;

				/* copy decompressed point to a temporary heap so we don't need to */
				/* decompress it again, this is a speed optimization! */

				ps.numpoints=1;
				ps.points=(kGUIFPoint2 *)m_sortpolysheap.Alloc(sizeof(kGUIFPoint2));
				memcpy(ps.points,m_ppoints,sizeof(kGUIFPoint2));

				/* save labels associated with this point */
				ps.numlabels=m_numlabels;
				memcpy(ps.curlabels,m_curlabels,sizeof(m_curlabels));

				m_sortpolys.SetEntry(m_numsortpolys++,ps);
			}
		}
	}
	if(sub->elements&ELEM_IDXPNT)
	{
		rstart=sub->m_idxstart;
		rend=sub->m_idxend;
		while(rstart<rend)
		{
			rstart=ReadPoint(sub,rstart);
			kGUI::DrawRect((int)m_ppoints[0].x,(int)m_ppoints[0].y,(int)m_ppoints[0].x+2,(int)m_ppoints[0].y+2,DrawColor(0,96,224));

			if(m_numlabels && GetZoom()>13)
			{
				//save text to be drawn after roads etc.
				ps.map=this;
				ps.polytype=m_polytype;
				ps.corners=m_polycorners;

				/* copy decompressed point to a temporary heap so we don't need to */
				/* decompress it again, this is a speed optimization! */

				ps.numpoints=1;
				ps.points=(kGUIFPoint2 *)m_sortpolysheap.Alloc(sizeof(kGUIFPoint2));
				memcpy(ps.points,m_ppoints,sizeof(kGUIFPoint2));

				/* save labels associated with this point */
				ps.numlabels=m_numlabels;
				memcpy(ps.curlabels,m_curlabels,sizeof(m_curlabels));

				m_sortpolys.SetEntry(m_numsortpolys++,ps);
			}
		}
	}
}

#if 0
void MSGPXMap::DrawTrainTracks(int nvert,kGUIFPoint2 *point,kGUIColor c)
{
	int i;
	float x1,y1,x2,y2;

	for(i=0;i<(nvert-1);++i)
	{
		x1=point[i].x;
		y1=point[i].y;
		x2=point[i+1].x;
		y2=point[i+1].y;

		if(kGUI::DrawLine(x1,y1,x2,y2,c)==true)
		{
			float dx,dy;
			float length,lcross;
			float curx,cury;
			float stepx,stepy,step;
			float offx,offy;

			dx=x2-x1;
			dy=y2-y1;

			if(fabs(dx)>fabs(dy))
			{
				length=fabs(dx);
				stepy=dy/fabs(dx);
				if(dx>0)
					stepx=1.0f;
				else
					stepx=-1.0f;
			}
			else
			{
				length=fabs(dy);
				stepx=dx/fabs(dy);
				if(dy>0)
					stepy=1.0f;
				else if(dy<0)
					stepy=-1.0f;
				else
					stepy=0.0f;
			}
			offx=stepy*3.0f;
			offy=stepx*3.0f;

			curx=x1;
			cury=y1;
			lcross=0.0f;
			step=(float)hypot(stepx,stepy);
			/* draw a cross line every 4 pixels */
			while(length>0.0f)
			{
				lcross+=step;
				if(lcross>=4.0f)
				{
					kGUI::DrawLine(curx+offx,cury-offy,curx-offx,cury+offy,c);
					lcross-=4.0f;
				}
				curx+=stepx;
				cury+=stepy;
				length-=1.0f;
			}
		}
	}
}
#endif

void MSGPXMap::DrawDashedLine(int nvert,kGUIFPoint2 *point,float segment,kGUIColor c)
{
	int i;
	float x1,y1,x2,y2,dx,dy,stepx,stepy,heading;
//	float segrem=0.0f;
	float seglen;
	bool draw=true;

	for(i=0;i<(nvert-1);++i)
	{
		x1=point[i].x;
		y1=point[i].y;
		x2=point[i+1].x;
		y2=point[i+1].y;
		dx=x2-x1;
		dy=y2-y1;
		heading=atan2(dx,dy);
		stepx=sin(heading);
		stepy=cos(heading);
		seglen=(float)hypot(dx,dy);

		while(seglen>=segment)
		{
			x2=x1+stepx*segment;
			y2=y1+stepy*segment;
			if(draw)
				kGUI::DrawLine(x1,y1,x2,y2,c);
			draw=!draw;
			x1=x2;
			y1=y2;
			seglen-=segment;
		}
		/* handle remainders */
	}
}

#if 1
void MSGPXMap::DrawTrainTracks(int nvert,kGUIFPoint2 *point,kGUIColor c)
{
	int i;
	float x0,y0,x1,y1,x2,y2,dx,dy,stepx,stepy,heading;
	float offx,offy,cx,cy;
	float segrem=0.0f;
	float segment=4.0f;
	float seglen;
//	bool draw=true;

	x0=point[0].x;
	y0=point[0].y;
	for(i=0;i<(nvert-1);++i)
	{
		x1=point[i].x;
		y1=point[i].y;
		x2=point[i+1].x;
		y2=point[i+1].y;
		kGUI::DrawLine(x1,y1,x2,y2,c);
		dx=x2-x1;
		dy=y2-y1;
		heading=atan2(dx,dy);
		stepx=sin(heading);
		stepy=cos(heading);
		seglen=(float)hypot(dx,dy);

		while((segrem+seglen)>=segment)
		{
			x2=x1+stepx*(segment-segrem);
			y2=y1+stepy*(segment-segrem);

			/* draw track cross line */
			offx=sin(heading+(PI/2.0f))*3.0f;
			offy=cos(heading+(PI/2.0f))*3.0f;
			cx=(x0+x2)/2.0f;
			cy=(y0+y2)/2.0f;
			kGUI::DrawLine(cx+offx,cy+offy,cx-offx,cy-offy,c);
			x0=x1=x2;
			y0=y1=y2;
			seglen-=(segment-segrem);
			segrem=0;
		}
		segrem+=seglen;
	}
}
#endif

void MSGPXMap::DrawDottedLine(int nvert,kGUIFPoint2 *point,float segment,float radius,kGUIColor c)
{
	int i;
	float x0,y0,x1,y1,x2,y2,dx,dy,stepx,stepy,heading;
	float segrem=0.0f;
	float seglen;
//	bool draw=true;

	x0=point[0].x;
	y0=point[0].y;
	for(i=0;i<(nvert-1);++i)
	{
		x1=point[i].x;
		y1=point[i].y;
		x2=point[i+1].x;
		y2=point[i+1].y;
		dx=x2-x1;
		dy=y2-y1;
		heading=atan2(dx,dy);
		stepx=sin(heading);
		stepy=cos(heading);
		seglen=(float)hypot(dx,dy);

		while((segrem+seglen)>=segment)
		{
			x2=x1+stepx*(segment-segrem);
			y2=y1+stepy*(segment-segrem);
			kGUI::DrawCircle((x0+x2)/2.0f,(y0+y2)/2.0f,radius+0.5f,c,0.33f);
			kGUI::DrawCircle((x0+x2)/2.0f,(y0+y2)/2.0f,radius,c,1.0f);
			x0=x1=x2;
			y0=y1=y2;
			seglen-=(segment-segrem);
			segrem=0;
		}
		segrem+=seglen;
	}
}


/* read label into string class */

enum
{
ENC6_FIRST,
ENC6_NORMAL,
ENC6_SYMBOL,
ENC6_SPECIAL
};

static char enc6[4][128]={
	{" ABCDEFGHIJKLMNOPQRSTUVWXYZ~~~~ 0123456789~~~~~~xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"},
	{" abcdefghijklmnopqrstuvwxyz~~~  0123456789~~~~~~xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"},
	{"@!\"#$%&'()*+,-./~~~~~~~~~~:;<=>?~~~~~~~~~~~[\\]^_yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"},
	{"`abcdefghijklmnopqrstuvwxyz~~~~~0123456789~~~~~~zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"}};

void MSGPXMap::ReadLabel(const char *enc,kGUIString *s)
{
	unsigned char c;
	char ch;
	int set=ENC6_FIRST;
	int numdec=0;
	int decindex=0;
	unsigned char dec[4];
	const unsigned char *uenc=(const unsigned char *)enc;
	
	s->Clear();
	if(!uenc[0])
		return;
	m_labelicon=-1;
	if(m_labelencoding==9)
	{
		while(uenc[0])
		{
			s->Append(uenc[0]);
			++uenc;
		}
		return;
	}

	/* 6 */
	while(1)
	{
		if(!numdec)
		{
			/* convert 3 bytes into 4 characters */
			dec[0]=uenc[0]>>2;
			dec[1]=((uenc[0]&0x03)<<4)|(uenc[1]>>4);
			dec[2]=((uenc[1]&0x0f)<<2)|(uenc[2]>>6);
			dec[3]=(uenc[2]&0x3f);
			decindex=0;
			numdec=4;
			uenc+=3;
		}
		/* read from the decoded buffer */
		c=dec[decindex++];
		--numdec;

		if(c>0x2f)
		{
			if(strstr(s->GetString(),"~"))
				c=99;
			return;
		}
		if(c>=0x2a && c<=0x2f)
		{
			//0x2A Interstate highway shield
			//0x2B U.S. highway shield
			//0x2C State highway (circle)
			//0x2D Canadian national highway, blue and red box
			//0x2E Canadian national highway, black and white box
			//0x2F Highway with small, white box
			m_labelicon=c-0x2a;
		}
		else
		{
			ch=0;
			if(c==0x1b)
				set=ENC6_SPECIAL;
			else if(c==0x1c)
				set=ENC6_SYMBOL;
			else if(c==0x1e)
			{
				ch=' ';
				set=ENC6_FIRST;
			}
			else if (set==ENC6_FIRST)
			{
				ch=enc6[ENC6_FIRST][c];
				set=ENC6_NORMAL;
			}
			else if (set==ENC6_NORMAL)
			{
				ch=enc6[ENC6_NORMAL][c];
				if(ch==' ')
					set=ENC6_FIRST;
			}
			else if(set==ENC6_SYMBOL)
			{
				ch=enc6[ENC6_SYMBOL][c];
				set=ENC6_FIRST;
			}
			else
			{
				ch=enc6[ENC6_SPECIAL][c];
				set=ENC6_FIRST;
			}
			if(ch)
				s->Append(ch);
		}
	}
}

void MSGPXMap::DrawLineLabel(kGUIText *t,int nvert,kGUIFPoint2 *point,double over,bool root)
{
	int s;
	int lsec;			/* longest section */
	double x1,y1,x2,y2;
	int nc=t->GetLen();		/* get length of label in characters */
	double lw=(double)t->GetWidth();	/* get width of label in pixels */
	double lh=(double)t->GetLineHeight();	/* get height of label in pixels */
	kGUIFPoint2 *p;
	double ldist;		/* longest distance */
	double dist;
	double cx,cy;
	double heading;

	if(!nc)
		return;	/* no string! */

	if((root==true) && (m_pixlen>400))
	{
		int pixcount=0;
		int ss=0;
		int numsplits=m_pixlen/200;
		int persplit;

		/* split line into sections */

		persplit=m_pixlen/numsplits;
		for(s=1;s<nvert;++s)
		{
			pixcount+=(int)hypot(point[s-1].y-point[s].y,point[s-1].x-point[s].x);
			if(pixcount>persplit)
			{
				pixcount-=persplit;
				DrawLineLabel(t,s-ss,point+ss,over,false);
				ss=s;
			}
		}

		return;
	}

	/* calculate longest section */

	lsec=0;
	ldist=hypot(point[0].y-point[1].y,point[0].x-point[1].x);
	p=point+1;

	for(s=1;s<nvert-1;++s)
	{
		dist=hypot((p->y)-((p+1)->y),(p->x)-((p+1)->x));
		if(dist>ldist)
		{
			ldist=dist;
			lsec=s;
		}
		++p;
	}

	if(lw>(ldist*2.0f))
		return;	/* section is not long enough */

	x1=point[lsec].x;
	y1=point[lsec].y;
	x2=point[lsec+1].x;
	y2=point[lsec+1].y;
	heading=atan2((point[lsec+1].y-point[lsec].y),(point[lsec].x-point[lsec+1].x));
	if(heading<0.0f)
		heading+=2*PI;

	if(heading>=(PI/2.0f) && heading<=((3.0f*PI)/2.0f))
		heading+=PI;

	/* start at center of longest segment */
	cx=(x1+x2)/2;
	cy=(y1+y2)/2;

	/* if this is a highway icon then don't rotate and center it over the road */
	if(m_labelicon>=0)
		heading=0;
	else
	{
		/* back 1/2 length of string */
		cx-=(cos(heading)*(lw*0.5f));
		cy+=(sin(heading)*(lw*0.5f));

		/* move a few pixels below the road */
		cx+=(cos(heading-(PI*2*0.25f))*over);
		cy-=(sin(heading-(PI*2*0.25f))*over);
	}
	DrawLabel(t,cx,cy,lw,lh,heading,true);
}

/* draw label but first check too make sure it doesn't */
/* overlap and previously drawn labels */

void MSGPXMap::DrawLabel(kGUIText *t,double lx,double ly,double lw,double lh,double heading,bool clipedge)
{
	int hx,hy,i;
	bool dodraw;
	kGUIPoint2 corners[4];
	kGUICorners bounds;
	kGUIImage *icon;
	double icx=0.0f,icy=0.0f;

	/* calculate 4 corners of the text */
	
	if(m_labelicon>=0)
	{
		kGUICorners tbounds;

		icon=m_icons.GetEntry(m_labelicon);
		/* center icon over position */
		lx-=icon->GetImageWidth()>>1;
		ly-=icon->GetImageHeight()>>1;
		icx=iconcenterx[m_labelicon];
		icy=iconcentery[m_labelicon];

		/* if icon is cliped against edge of tile then don't draw */
		bounds.lx=(int)lx;
		bounds.rx=(int)lx+icon->GetImageWidth();
		bounds.ty=(int)ly;
		bounds.by=(int)ly+icon->GetImageHeight();

		tbounds.lx=0;
		tbounds.ty=0;
		tbounds.rx=256;
		tbounds.by=256;

		if(clipedge)
		{
			if(kGUI::Inside(&bounds,&tbounds)==false)
				return;
		}
		else
		{
			if(kGUI::Overlap(&bounds,&tbounds)==false)
				return;
		}
	}
	else
		icon=0;

	if(heading==0.0f)
	{
		corners[0].x=(int)lx;
		corners[0].y=(int)ly;
		corners[1].x=(int)(lx+lw);
		corners[1].y=(int)ly;
		corners[2].x=(int)(lx+lw);
		corners[2].y=(int)(ly+lh);
		corners[3].x=(int)lx;
		corners[3].y=(int)(ly+lh);
		
		bounds.lx=(int)lx;
		bounds.rx=(int)(lx+lw);
		bounds.ty=(int)ly;
		bounds.by=(int)(ly+lh);
	}
	else
	{
		corners[0].x=(int)lx;
		corners[0].y=(int)ly;
		corners[1].x=(int)(lx+(int)(cos(heading)*lw));
		corners[1].y=(int)(ly-(int)(sin(heading)*lw));
		hx=(int)(cos(heading-(PI*2*0.25f))*lh);
		hy=(int)(sin(heading-(PI*2*0.25f))*lh);
		corners[3].x=corners[0].x+hx;
		corners[3].y=corners[0].y-hy;
		corners[2].x=corners[1].x+hx;
		corners[2].y=corners[1].y-hy;

		bounds.lx=MIN(MIN(corners[0].x,corners[1].x),
						MIN(corners[2].x,corners[3].x));
		bounds.ty=MIN(MIN(corners[0].y,corners[1].y),
						MIN(corners[2].y,corners[3].y));
		bounds.rx=MAX(MAX(corners[0].x,corners[1].x),
						MAX(corners[2].x,corners[3].x));
		bounds.by=MAX(MAX(corners[0].y,corners[1].y),
						MAX(corners[2].y,corners[3].y));
	}

	dodraw=m_lc.Check(&bounds,corners,clipedge);

	if(dodraw==true)
	{
		if(heading==0.0f)
		{
//0x2A Interstate highway shield
//0x2B U.S. highway shield
//0x2C State highway (circle)
//0x2D Canadian national highway, blue and red box
//0x2E Canadian national highway, black and white box
//0x2F Highway with small, white box
			if(icon)
			{
				ICON_POS *ipp;
				ICON_POS ip;

				/* if icon too close to another one for same highway then ignore, else add to list */
				for(i=0;(i<m_numiconsdrawn) && icon;++i)
				{
					ipp=m_iconpos.GetEntryPtr(i);
					if(ipp->icon==m_labelicon)
					{
						if(hypot(ipp->x-lx,ipp->y-ly)<128.0f)
							return;
					}
				}

				ip.x=(int)lx;
				ip.y=(int)ly;
				ip.icon=m_labelicon;
				m_iconpos.SetEntry(m_numiconsdrawn++,ip);

				if((lw+4)>icon->GetImageWidth())
				{
					icon->SetScale((double)(lw+4)/(double)icon->GetImageWidth(),1.0f);
					icon->Draw(0,(int)lx,(int)ly);
					ly+=icy-(lh/2);
					lx+=2;
				}
				else
				{
					icon->SetScale(1.0f,1.0f);
					icon->Draw(0,(int)lx,(int)ly);
					lx+=icx-(lw/2);
					ly+=icy-(lh/2);
				}

			}
			t->DrawRot((float)lx,(float)ly,0.0f,DrawColor(0,0,0),1.0f);
		}
		else
			t->DrawRot((float)lx,(float)ly,(float)heading,DrawColor(0,0,0),1.0f);
	}
}

int maxpp=0;

/* used for both polygons and polylines */
const char *MSGPXMap::ReadPoly(MSSUBDIV *sub,const char *rstart,int type)
{
	int pi,npi;	/* point index */
	unsigned int polydata;
	bool wordlen;
	int obj_size;
	int blen,dx,dy,xtrabit;
	double ddx,ddy,lddx=0.0f,lddy=0.0f;
	int bitinfo;
	kGUIBitStream bs;
	int nbits;
	int xsign,ysign;
	int xbits,ybits;
	int totalbits;
	int shiftby=sub->shiftby;
	int offset;
	int xoffset;
	int loffset;
	double pixlen;	/* length of perimeter in pixels */
	int road_data;
	int road_length;
	GPXCoord c;
	const char *oldrstart=rstart;
	double tx=m_tx;
	double ty=m_ty;

	m_numlabels=0;	/* default is no labels */
	polydata= ReadU32(rstart);
	if(type==ELEM_POLYLINE)
		m_polytype=(polydata & 0x3f);
	else
		m_polytype=(polydata & 0x7f);

	wordlen= (bool) ((polydata & 0x80)!=0);
	if(polydata & 0x40000000)
		xtrabit=1;
	else
		xtrabit=0;

	dx= (Read16(rstart+4))<<shiftby;
	dy= (Read16(rstart+6))<<shiftby;

	m_points[0].dlat= sub->center.dlat + dy;
	m_points[0].dlong= sub->center.dlong + dx;

	if ( polydata & 0x80000000 )
	{
		offset=(polydata & 0x3FFFFF00)>>8;
		if(offset && m_netsize)
		{
			//assert(offset<m_netsize,"Bad offset!");		
			offset<<=m_netshift;
			if(offset<m_netsize)
			{
				do
				{
					xoffset=ReadU24(m_netstart+offset);
					if(xoffset&0x400000)
					{
						/* segmented road offset? */
						loffset=0;
					}
					else
					{
						loffset=xoffset&0x3FFFFF;
						loffset<<=m_labelshift;
						assert(loffset<m_labelsize,"bad offset!");
						if(loffset<m_labelsize)
						{
							assert(m_numlabels<MAXLABELS,"Too many Labels!");
							m_curlabels[m_numlabels++]=m_labelstart+loffset;
						}
					}
					offset+=3;
				}while(!(xoffset&0x800000));
				road_data=ReadU8(m_netstart+offset);
				road_length=ReadU24(m_netstart+offset+1);
			}
		}
	}
	else
	{
		offset=(polydata & 0x3FFFFF00)>>8;
		if(offset)
		{
	//		assert(offset<m_labelsize,"bad offset!");
			offset<<=m_labelshift;
			if(offset<m_labelsize)
			{
				assert(m_numlabels<MAXLABELS,"Too many Labels!");
				m_curlabels[m_numlabels++]=m_labelstart+offset;
			}
		}
	}

	obj_size= 8;

	if ( wordlen )
	{
		blen= (int) ReadU16(rstart+8);
		rstart+= 10;
		obj_size+= 3+blen;
	}
	else
	{
		blen= (int) ReadU8(rstart+8);
		rstart+= 9;
		obj_size+= 2+blen;
	}

#if 1
		if( (m_points[0].dlat<sub->boundary.se.dlat) || 
			(m_points[0].dlat>sub->boundary.nw.dlat) ||
			(m_points[0].dlong>sub->boundary.se.dlong) ||
			(m_points[0].dlong<sub->boundary.nw.dlong))
		{
			DebugPrint("Center overflow, %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",oldrstart[0]&255,oldrstart[1]&255,oldrstart[2]&255,oldrstart[3]&255,oldrstart[4]&255,oldrstart[5]&255,oldrstart[6]&255,oldrstart[7]&255,oldrstart[8]&255,oldrstart[9]&255);
			return(0);
//					goto skippoly;
		}
#endif

	bitinfo= ReadU8(rstart++);
	totalbits=blen<<3;
	bs.Set(rstart);
	if(bs.ReadU(1))
		xsign=bs.ReadU(1)==1?-1:1;
	else
		xsign=0;
	if(bs.ReadU(1))
		ysign=bs.ReadU(1)==1?-1:1;
	else
		ysign=0;

	/* calculate the number of bits for each x coord */
	nbits=bitinfo&0xF;
	if(nbits>9)
		nbits=(nbits<<1)-9;
	if(!xsign)
		++nbits;	/* need to get extra sign bit */
	xbits= nbits+2;
	/* calculate the number of bits for each y coord */
	nbits=bitinfo>>4;
	if(nbits>9)
		nbits=(nbits<<1)-9;
	if(!ysign)
		++nbits;	/* need to get extra sign bit */
	ybits= nbits+2;
	pi=1;
	do
	{
		if (xtrabit)
			bs.ReadU(1);	/* what is it for? */

		dx=GetPoint(&bs,xbits,xsign);
		dy=GetPoint(&bs,ybits,ysign);
		/* relative to previous one */
		if(pi>=MAXPP)
		{
			DebugPrint("Aborting, too many poinits!\n");
			return(0);
		}
		assert(pi<MAXPP,"Overflow!");
		m_points[pi].dlat= m_points[pi-1].dlat + (dy<<shiftby);
		m_points[pi].dlong= m_points[pi-1].dlong + (dx<<shiftby);

#if 1
		if( (m_points[pi].dlat<sub->boundary.se.dlat) || 
			(m_points[pi].dlat>sub->boundary.nw.dlat) ||
			(m_points[pi].dlong>sub->boundary.se.dlong) ||
			(m_points[pi].dlong<sub->boundary.nw.dlong))
		{
			DebugPrint("Aborting, poly outside of zone\n");
			return(0);
		}
#endif

		++pi;
	}while((bs.NumRead()+xbits+ybits+xtrabit)<=totalbits);
	
	/* convert the points from lon/lat to map pixels */
	/* skip duplicate points as they DO occur */

	pixlen=0.0f;
	npi=0;
	for(int x=0;x<pi;++x)
	{
		c.Set(ToDegrees(m_points[x].dlat),ToDegrees(m_points[x].dlong));
		
		ToMap(&c,&ddx,&ddy);
		m_ppoints[npi].x=(float)(ddx-tx);
		m_ppoints[npi].y=(float)(ddy-ty);
		dx=(int)ddx;
		dy=(int)ddy;
		if(!x)
		{
			++npi;
			m_polycorners.lx=m_polycorners.rx=dx;
			m_polycorners.ty=m_polycorners.by=dy;
		}
		else
		{
			if(ddx!=lddx || ddy!=lddy)
			{
				++npi;
				pixlen+=(int)hypot(ddx-lddx,ddy-lddy);
				if(dx<m_polycorners.lx)
					m_polycorners.lx=dx;
				if(dy<m_polycorners.ty)
					m_polycorners.ty=dy;
				if(dx>m_polycorners.rx)
					m_polycorners.rx=dx;
				if(dy>m_polycorners.by)
					m_polycorners.by=dy;
			}
		}
		lddx=ddx;
		lddy=ddy;
	}
	m_numpoints=npi;
	m_pixlen=(int)pixlen;	/* todo, change calc to use double in the draw label code */
	rstart+=blen;
	if(pi>maxpp)
		maxpp=pi;
	return(rstart);
}

const char *MSGPXMap::ReadPoint(MSSUBDIV *sub,const char *rstart)
{
	int pointdata, lbloffset;
	int type;
	int dx, dy;
	GPXCoord c;
	bool has_subtype,is_poi;
	int loffset;

	m_numlabels=0;
	pointdata= ReadU32(rstart);
	type= (pointdata & 0xFF);
	lbloffset= (pointdata & 0xFFFFF00)>>8;

	dx= (Read16(rstart+4))<<sub->shiftby;
	dy= (Read16(rstart+6))<<sub->shiftby;

//	printf("dx= %-8.3f  dy= %-8.3f\n", todegrees(dx), todegrees(dy));

	m_points[0].dlat= sub->center.dlat + dy;
	m_points[0].dlong= sub->center.dlong + dx;

//	printf("pointdata= %08x\n", pointdata);
	if ( pointdata & 0x80000000 )
	{
//				subtype= ReadU8(rstart+8);
		has_subtype= true;
	}
	else
	{
		has_subtype= false;
	}

	if ( pointdata & 0x40000000 )
	{
//		printf("POI\n");
		is_poi= true;
//				addr= img->lbl->get_poi_addr(lbloffset);
//				label= addr.name;
	}
	else
	{
		is_poi= false;
//		hex_dump(stdout, &pointdata, 4);
//		printf("label offset= %p\n", lbloffset);
//				label= img->lbl->get_label_at(lbloffset);

		loffset=lbloffset&0x3FFFFF;
		loffset<<=m_labelshift;
		assert(loffset<m_labelsize,"bad offset!");
		if(loffset<m_labelsize)
		{
			assert(m_numlabels<MAXLABELS,"Too many Labels!");
			m_curlabels[m_numlabels++]=m_labelstart+loffset;
		}
	}

	/* project and draw it */
	c.Set(ToDegrees(m_points[0].dlat),ToDegrees(m_points[0].dlong));

	ToMap(&c,&dx,&dy);
	m_ppoints[0].x=(float)(dx-m_tx);
	m_ppoints[0].y=(float)(dy-m_ty);
	if(has_subtype==true)
		rstart+=9;
	else
		rstart+=8;
	return(rstart);
}

MSGPXChild::~MSGPXChild()
{
	if(m_map)
		delete m_map;
}


