/* -------------------------------------------------------------------------------------------------
Copyright (c) 2008 Andrew Green
http://www.zoolib.org

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------------------- */

#ifndef __ZGRgnRep_XRegion_h__
#define __ZGRgnRep_XRegion_h__ 1
#include "zconfig.h"

#include "zoolib/ZCONFIG_SPI.h"
#include "zoolib/ZCONFIG_API.h"

#ifndef ZCONFIG_API_Avail__GRgnRep_XRegion
	#define ZCONFIG_API_Avail__GRgnRep_XRegion ZCONFIG_SPI_Enabled(X11)
#endif

#ifndef ZCONFIG_API_Desired__GRgnRep_XRegion
	#define ZCONFIG_API_Desired__GRgnRep_XRegion 1
#endif

#include "zoolib/ZGRgn.h"

#if ZCONFIG_API_Enabled(GRgnRep_XRegion)

ZMACRO_MSVCStaticLib_Reference(GRgnRep_XRegion)

#include "zoolib/ZCompat_Xlib.h"
#include <X11/Xutil.h>

namespace ZooLib {

// =================================================================================================
// MARK: - ZGRgnRep_XRegion

class ZGRgnRep_XRegion : public ZGRgnRep
	{
public:
	static ZRef<ZGRgnRep_XRegion> sGetRep(const ZRef<ZGRgnRep>& iRep);

	ZGRgnRep_XRegion(Region iRegion, bool iAdopt);
	ZGRgnRep_XRegion(Region iRegion);
	virtual ~ZGRgnRep_XRegion();

// From ZGRgnRep
	virtual size_t Decompose(DecomposeProc iProc, void* iRefcon);

	virtual bool Contains(ZCoord iH, ZCoord iV);
	virtual bool IsEmpty();
	virtual ZRectPOD Bounds();
	virtual bool IsSimpleRect();
	virtual bool IsEqualTo(const ZRef<ZGRgnRep>& iRep);

	virtual void Inset(ZCoord iH, ZCoord iV);
	virtual ZRef<ZGRgnRep> Insetted(ZCoord iH, ZCoord iV);

	virtual void Offset(ZCoord iH, ZCoord iV);
	virtual ZRef<ZGRgnRep> Offsetted(ZCoord iH, ZCoord iV);

	virtual void Include(const ZRectPOD& iRect);
	virtual ZRef<ZGRgnRep> Including(const ZRectPOD& iRect);

	virtual void Intersect(const ZRectPOD& iRect);
	virtual ZRef<ZGRgnRep> Intersecting(const ZRectPOD& iRect);

	virtual void Exclude(const ZRectPOD& iRect);
	virtual ZRef<ZGRgnRep> Excluding(const ZRectPOD& iRect);

	virtual void Include(const ZRef<ZGRgnRep>& iRep);
	virtual ZRef<ZGRgnRep> Including(const ZRef<ZGRgnRep>& iRep);

	virtual void Intersect(const ZRef<ZGRgnRep>& iRep);
	virtual ZRef<ZGRgnRep> Intersecting(const ZRef<ZGRgnRep>& iRep);

	virtual void Exclude(const ZRef<ZGRgnRep>& iRep);
	virtual ZRef<ZGRgnRep> Excluding(const ZRef<ZGRgnRep>& iRep);

	virtual void Xor(const ZRef<ZGRgnRep>& iRep);
	virtual ZRef<ZGRgnRep> Xoring(const ZRef<ZGRgnRep>& iRep);

// Our protocol
	Region GetRegion();

private:
	Region fRegion;
	};

} // namespace ZooLib

#endif // ZCONFIG_API_Enabled(GRgnRep_XRegion)

#endif // __ZGRgnRep_XRegion_h__
