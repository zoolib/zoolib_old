/* -------------------------------------------------------------------------------------------------
Copyright (c) 2009 Andrew Green
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

#include "zoolib/ZYad_XMLAttr.h"

/*
ZYad_XMLAttr turns this:
  <tag1>
    <tag2>val3</tag2>
    <tag4 at5="5" at6="6"/>
    <tag7 at8="8">
      <tag9/>
      <tag10></tag10>
    </tag7>
  </tag1>

into this:
  {
  tag1 =
    {
    tag2 = "val3";
    tag4 =
      {
      at5 = "5";
      at6 = "6";
      };
    tag7 =
      {
      at8 = "8";
      tag9 = {};
      tag10 = "";
      };
    };
  }
*/

namespace ZooLib {
namespace ZYad_XMLAttr {

using std::pair;
using std::string;

// =================================================================================================
// MARK: - Static parsing functions

static void spThrowParseException(const string& iMessage)
	{
	throw ParseException(iMessage);
	}

// =================================================================================================
// MARK: - ParseException

ParseException::ParseException(const string& iWhat)
:	ZYadParseException(iWhat)
	{}

ParseException::ParseException(const char* iWhat)
:	ZYadParseException(iWhat)
	{}

// =================================================================================================
// MARK: - YadMapR

class YadMapR : public ZYadMapR
	{
public:
	YadMapR(const ZML::Attrs_t& iAttrs);
	YadMapR(ZRef<ZML::StrimmerU> iStrimmerU);
	YadMapR(ZRef<ZML::StrimmerU> iStrimmerU, const string& iOuterName, const ZML::Attrs_t& iAttrs);

// From ZYadMapR
	virtual ZRef<ZYadR> ReadInc(Name& oName);

private:
	ZRef<ZML::StrimmerU> fStrimmerU;
	const string fOuterName;
	const ZML::Attrs_t fAttrs;
	ZML::Attrs_t::const_iterator fIter;
	};

YadMapR::YadMapR(const ZML::Attrs_t& iAttrs)
:	fAttrs(iAttrs)
,	fIter(fAttrs.begin())
	{}

YadMapR::YadMapR(ZRef<ZML::StrimmerU> iStrimmerU)
:	fStrimmerU(iStrimmerU)
,	fIter(fAttrs.begin())
	{}

YadMapR::YadMapR(
	ZRef<ZML::StrimmerU> iStrimmerU, const string& iOuterName, const ZML::Attrs_t& iAttrs)
:	fStrimmerU(iStrimmerU)
,	fOuterName(iOuterName)
,	fAttrs(iAttrs)
,	fIter(fAttrs.begin())
	{}

ZRef<ZYadR> YadMapR::ReadInc(Name& oName)
	{
	if (fIter != fAttrs.end())
		{
		oName = fIter->first;
		ZRef<ZYadR> result = ZooLib::sYadR(fIter->second);
		++fIter;
		return result;
		}

	if (!fStrimmerU)
		return null;

	ZML::StrimU& theR = fStrimmerU->GetStrim();

	// We have read the begin, and have returned
	// any attributes on that begin.
	sSkipText(theR);

	if (fOuterName.empty())
		{
		if (theR.Current() == ZML::eToken_Exhausted)
			return null;
		}
	else if (sTryRead_End(theR, fOuterName))
		{
		return null;
		}

	if (theR.Current() == ZML::eToken_TagEmpty)
		{
		oName = theR.Name();
		ZRef<ZYadR> result = new YadMapR(theR.Attrs());
		theR.Advance();
		return result;
		}

	if (theR.Current() != ZML::eToken_TagBegin)
		spThrowParseException("Expected begin tag");

	ZML::Attrs_t theAttrs = theR.Attrs();
	const string theName = theR.Name();
	theR.Advance();

	oName = theName;
	const string8 theText = theR.ReadAll8();
	sSkipText(theR);

	if (theR.Current() == ZML::eToken_TagEnd)
		{
		if (theR.Name() != theName)
			spThrowParseException("Expected end tag '" + theName + "'");

		if (theAttrs.empty())
			{
			theR.Advance();
			return ZooLib::sYadR(theText);
			}
		else if (!theText.empty())
			{
			theAttrs.push_back(pair<string, string>("!text", theText));
			}
		}

	return new YadMapR(fStrimmerU, theName, theAttrs);
	}

// =================================================================================================
// MARK: - sYadR

ZRef<ZYadR> sYadR(ZRef<ZML::StrimmerU> iStrimmerU)
	{
	sSkipText(iStrimmerU->GetStrim());
	if (iStrimmerU->GetStrim().Current() != ZML::eToken_Exhausted)
		return new YadMapR(iStrimmerU);
	return null;
	}

ZRef<ZYadR> sYadR(ZRef<ZML::StrimmerU> iStrimmerU,
	const string& iOuterName, const ZML::Attrs_t& iAttrs)
	{ return new YadMapR(iStrimmerU, iOuterName, iAttrs); }

} // namespace ZYad_XMLAttr
} // namespace ZooLib
