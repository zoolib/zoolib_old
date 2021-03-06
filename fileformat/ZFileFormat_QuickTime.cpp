/* -------------------------------------------------------------------------------------------------
Copyright (c) 2003 Andrew Green and Learning in Motion, Inc.
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

#include "zoolib/fileformat/ZFileFormat_QuickTime.h"
#include "zoolib/ZByteSwap.h"
#include "zoolib/ZDebug.h"

#include <string.h> // For strlen

#ifndef kDebug_QTFile
	#define kDebug_QTFile 1
#endif

namespace ZooLib {
namespace FileFormat {
namespace QuickTime {

using std::min;
using std::pair;
using std::vector;

// =================================================================================================
// MARK: - Writer

Writer::Writer()
	{}

Writer::~Writer()
	{
	ZAssertStop(1, fChunks.empty());
	}

void Writer::Begin(const ZStreamWPos& iStream, uint32 iChunkType)
	{
	fChunks.push_back(pair<uint64, size_t>(iStream.GetPosition(), iChunkType));
	iStream.WriteUInt32(0); // Dummy for header-including size to be written in End
	iStream.WriteUInt32(iChunkType);
	}

void Writer::Begin(const ZStreamWPos& iStream, const char* iChunkType)
	{
	ZAssertStop(kDebug_QTFile, ::strlen(iChunkType) == 4);
	uint32 chunkType = ZByteSwap_ReadBig32(iChunkType);
	this->Begin(iStream, chunkType);
	}

void Writer::End(const ZStreamWPos& iStream, uint32 iChunkType)
	{
	// Verify we've got a chunk open.
	ZAssertStop(kDebug_QTFile, fChunks.size());

	// Grab info about the currently open chunk, and remove it from the stack.
	uint64 start = fChunks.back().first;
	uint32 chunkType = fChunks.back().second;
	fChunks.pop_back();

	// Verify that the chunk we're closing is the last one we opened.
	ZAssertStop(kDebug_QTFile, chunkType == iChunkType);

	// Write the chunk's size.
	uint64 position = iStream.GetPosition();
	iStream.SetPosition(start);
	iStream.WriteUInt32(position - start);
	iStream.SetPosition(position);
	}

void Writer::End(const ZStreamWPos& iStream, const char* iChunkType)
	{
	ZAssertStop(kDebug_QTFile, ::strlen(iChunkType) == 4);
	uint32 chunkType = ZByteSwap_ReadBig32(iChunkType);
	this->End(iStream, chunkType);
	}

// =================================================================================================
// MARK: - StreamR_Chunk

StreamR_Chunk::StreamR_Chunk(uint32& oChunkType, const ZStreamR& iStream)
:	fStream(iStream)
	{ this->pInit(oChunkType, true); }

StreamR_Chunk::StreamR_Chunk(
	uint32& oChunkType, bool iSkipOnDestroy, const ZStreamR& iStream)
:	fStream(iStream)
	{ this->pInit(oChunkType, iSkipOnDestroy); }

StreamR_Chunk::~StreamR_Chunk()
	{
	if (fSkipOnDestroy)
		{
		try
			{
			fStream.Skip(fCountRemaining);
			}
		catch (...)
			{}
		}
	}

void StreamR_Chunk::Imp_Read(void* oDest, size_t iCount, size_t* oCountRead)
	{
	uint8* localDest = reinterpret_cast<uint8*>(oDest);

	while (iCount && fCountRemaining)
		{
		size_t countRead;
		fStream.Read(localDest, min(iCount, fCountRemaining), &countRead);
		if (countRead == 0)
			break;
		localDest += countRead;
		fCountRemaining -= countRead;
		iCount -= countRead;
		}

	if (oCountRead)
		*oCountRead = localDest - reinterpret_cast<uint8*>(oDest);
	}

size_t StreamR_Chunk::Imp_CountReadable()
	{ return min(fCountRemaining, fStream.CountReadable()); }

bool StreamR_Chunk::Imp_WaitReadable(double iTimeout)
	{ return fStream.WaitReadable(iTimeout); }

void StreamR_Chunk::Imp_CopyToDispatch(
	const ZStreamW& iStreamW, uint64 iCount, uint64* oCountRead, uint64* oCountWritten)
	{
	uint64 countRead;
	fStream.CopyTo(iStreamW, min(iCount, uint64(fCountRemaining)), &countRead, oCountWritten);
	fCountRemaining -= countRead;
	if (oCountRead)
		*oCountRead = countRead;
	}

void StreamR_Chunk::Imp_CopyTo(
	const ZStreamW& iStreamW, uint64 iCount, uint64* oCountRead, uint64* oCountWritten)
	{
	uint64 countRead;
	fStream.CopyTo(iStreamW, min(iCount, uint64(fCountRemaining)), &countRead, oCountWritten);
	fCountRemaining -= countRead;
	if (oCountRead)
		*oCountRead = countRead;
	}

void StreamR_Chunk::Imp_Skip(uint64 iCount, uint64* oCountSkipped)
	{
	uint64 countSkipped;
	fStream.Skip(min(iCount, uint64(fCountRemaining)), &countSkipped);
	fCountRemaining -= countSkipped;
	if (oCountSkipped)
		*oCountSkipped = countSkipped;
	}

void StreamR_Chunk::pInit(uint32& oChunkType, bool iSkipOnDestroy)
	{
	try
		{
		fSkipOnDestroy = iSkipOnDestroy;
		fCountRemaining = fStream.ReadUInt32() - 8;
		oChunkType = fStream.ReadUInt32();
		}
	catch (...)
		{
		fSkipOnDestroy = false;
		fCountRemaining = 0;
		oChunkType = 0;
		}
	}

// =================================================================================================
// MARK: - StreamRPos_Chunk

StreamRPos_Chunk::StreamRPos_Chunk(
	uint32& oChunkType, const ZStreamRPos& iStream)
:	fStream(iStream)
	{ this->pInit(oChunkType, true); }

StreamRPos_Chunk::StreamRPos_Chunk(
	uint32& oChunkType, bool iSkipOnDestroy, const ZStreamRPos& iStream)
:	fStream(iStream)
	{ this->pInit(oChunkType, iSkipOnDestroy); }

StreamRPos_Chunk::~StreamRPos_Chunk()
	{
	if (fSkipOnDestroy)
		{
		try
			{
			fStream.SetPosition(fStart + fSize);
			}
		catch (...)
			{}
		}
	}

void StreamRPos_Chunk::Imp_Read(
	void* oDest, size_t iCount, size_t* oCountRead)
	{
	uint8* localDest = reinterpret_cast<uint8*>(oDest);
	size_t countRemaining = ZStream::sClampedSize(iCount, fStart + fSize, fStream.GetPosition());
	while (countRemaining)
		{
		size_t countRead;
		fStream.Read(localDest, countRemaining, &countRead);
		if (countRead == 0)
			break;
		localDest += countRead;
		countRemaining -= countRead;
		}

	if (oCountRead)
		*oCountRead = localDest - reinterpret_cast<uint8*>(oDest);
	}

void StreamRPos_Chunk::Imp_CopyToDispatch(
	const ZStreamW& iStreamW, uint64 iCount, uint64* oCountRead, uint64* oCountWritten)
	{
	fStream.CopyTo(iStreamW,
		min(iCount, fStart + fSize - fStream.GetPosition()), oCountRead, oCountWritten);
	}

void StreamRPos_Chunk::Imp_CopyTo(
	const ZStreamW& iStreamW, uint64 iCount, uint64* oCountRead, uint64* oCountWritten)
	{
	fStream.CopyTo(iStreamW,
		min(iCount, fStart + fSize - fStream.GetPosition()), oCountRead, oCountWritten);
	}

void StreamRPos_Chunk::Imp_Skip(uint64 iCount, uint64* oCountSkipped)
	{
	if (uint64 countToSkip = ZStream::sClampedCount(iCount, fStart + fSize, fStream.GetPosition()))
		fStream.Skip(countToSkip, oCountSkipped);
	else if (oCountSkipped)
		*oCountSkipped = 0;
	}

uint64 StreamRPos_Chunk::Imp_GetPosition()
	{ return fStream.GetPosition() - fStart; }

void StreamRPos_Chunk::Imp_SetPosition(uint64 iPosition)
	{ fStream.SetPosition(fStart + iPosition); }

uint64 StreamRPos_Chunk::Imp_GetSize()
	{ return min(uint64(fSize), fStream.GetSize() - fStart); }

void StreamRPos_Chunk::pInit(uint32& oChunkType, bool iSkipOnDestroy)
	{
	try
		{
		fSkipOnDestroy = iSkipOnDestroy;
		fSize = fStream.ReadUInt32() - 8;
		oChunkType = fStream.ReadUInt32();
		}
	catch (...)
		{
		fSkipOnDestroy = false;
		fSize = 0;
		oChunkType = 0;
		}
	fStart = fStream.GetPosition();
	}

// =================================================================================================
// MARK: - StreamWPos_Chunk

StreamWPos_Chunk::StreamWPos_Chunk(
	uint32 iChunkType, const ZStreamWPos& iStream)
:	fStream(iStream)
	{
	fStream.WriteUInt32(0); // Dummy for header-including size to be written in End
	fStream.WriteUInt32(iChunkType);
	fStart = fStream.GetPosition();
	}

StreamWPos_Chunk::~StreamWPos_Chunk()
	{
	uint64 position = fStream.GetPosition();
	fStream.SetPosition(fStart - 8);
	fStream.WriteUInt32(position - fStart + 8);
	fStream.SetPosition(position);
	}

void StreamWPos_Chunk::Imp_Write(
	const void* iSource, size_t iCount, size_t* oCountWritten)
	{ fStream.Write(iSource, iCount, oCountWritten); }

void StreamWPos_Chunk::Imp_CopyFromDispatch(
	const ZStreamR& iStreamR, uint64 iCount, uint64* oCountRead, uint64* oCountWritten)
	{ fStream.CopyFrom(iStreamR, iCount, oCountRead, oCountWritten); }

void StreamWPos_Chunk::Imp_CopyFrom(
	const ZStreamR& iStreamR, uint64 iCount, uint64* oCountRead, uint64* oCountWritten)
	{ fStream.CopyFrom(iStreamR, iCount, oCountRead, oCountWritten); }

void StreamWPos_Chunk::Imp_Flush()
	{ fStream.Flush(); }

uint64 StreamWPos_Chunk::Imp_GetPosition()
	{ return fStream.GetPosition() - fStart; }

void StreamWPos_Chunk::Imp_SetPosition(uint64 iPosition)
	{ fStream.SetPosition(iPosition + fStart); }

uint64 StreamWPos_Chunk::Imp_GetSize()
	{ return fStream.GetSize() - fStart; }

void StreamWPos_Chunk::Imp_SetSize(uint64 iSize)
	{ fStream.SetSize(iSize + fStart); }

} // namespace QuickTime
} // namespace FileFormat
} // namespace ZooLib
