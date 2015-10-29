// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "syzygy/msf/msf_byte_stream.h"

#include <algorithm>

#include "gtest/gtest.h"

namespace msf {

namespace {

class TestMsfByteStream : public MsfByteStream {
 public:
  TestMsfByteStream() : MsfByteStream() {}

  using MsfByteStream::ReadBytes;
};

class TestMsfStream : public MsfStream {
 public:
  explicit TestMsfStream(size_t length) : MsfStream(length) {}

  virtual ~TestMsfStream() {}

  bool ReadBytes(void* dest, size_t count, size_t* bytes_read) {
    DCHECK(dest != NULL);
    DCHECK(bytes_read != NULL);

    if (pos() == length()) {
      *bytes_read = 0;
      return true;
    }

    count = std::min(count, length() - pos());
    ::memset(dest, 0xFF, count);
    Seek(pos() + count);
    *bytes_read = count;

    return true;
  }
};

}  // namespace

TEST(MsfByteStreamTest, InitFromByteArray) {
  uint8 data[] = {1, 2, 3, 4, 5, 6, 7, 8};

  scoped_refptr<MsfByteStream> stream(new MsfByteStream());
  EXPECT_TRUE(stream->Init(data, arraysize(data)));
  EXPECT_EQ(arraysize(data), stream->length());
  EXPECT_TRUE(stream->data() != NULL);

  for (size_t i = 0; i < stream->length(); ++i) {
    uint8 num = 0;
    EXPECT_TRUE(stream->Read(&num, 1));
    EXPECT_EQ(data[i], num);
  }
}

TEST(MsfByteStreamTest, InitFromMsfStream) {
  scoped_refptr<TestMsfStream> test_stream(new TestMsfStream(64));

  scoped_refptr<MsfByteStream> stream(new MsfByteStream());
  EXPECT_TRUE(stream->Init(test_stream.get()));
  EXPECT_EQ(test_stream->length(), stream->length());
  EXPECT_TRUE(stream->data() != NULL);

  for (size_t i = 0; i < stream->length(); ++i) {
    uint8 num = 0;
    EXPECT_TRUE(stream->Read(&num, 1));
    EXPECT_EQ(0xFF, num);
  }
}

TEST(MsfByteStreamTest, InitFromMsfStreamPart) {
  uint8 data[] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};
  scoped_refptr<MsfByteStream> test_stream(new MsfByteStream());
  EXPECT_TRUE(test_stream->Init(data, arraysize(data)));

  scoped_refptr<MsfByteStream> stream(new MsfByteStream());
  EXPECT_TRUE(test_stream->Seek(2));
  EXPECT_TRUE(stream->Init(test_stream.get(), 7));
  EXPECT_EQ(7, stream->length());
  EXPECT_TRUE(stream->data() != NULL);

  for (size_t i = 0; i < stream->length(); ++i) {
    uint8 num = 0;
    EXPECT_TRUE(stream->Read(&num, 1));
    EXPECT_EQ(data[i + 2], num);
  }
}

TEST(MsfByteStreamTest, ReadBytes) {
  size_t len = 17;
  scoped_refptr<TestMsfStream> test_stream(new TestMsfStream(len));

  scoped_refptr<TestMsfByteStream> stream(new TestMsfByteStream());
  EXPECT_TRUE(stream->Init(test_stream.get()));

  int total_bytes = 0;
  while (true) {
    uint8 buffer[4];
    size_t bytes_read = 0;
    EXPECT_TRUE(stream->ReadBytes(buffer, sizeof(buffer), &bytes_read));
    if (bytes_read == 0)
      break;
    total_bytes += bytes_read;
  }

  EXPECT_EQ(len, total_bytes);
}

TEST(MsfByteStreamTest, GetWritableStream) {
  scoped_refptr<MsfStream> stream(new MsfByteStream());
  scoped_refptr<WritableMsfStream> writer1 = stream->GetWritableStream();
  EXPECT_TRUE(writer1.get() != NULL);

  // NOTE: This is a condition that only needs to be true currently because
  //     of limitations in the WritableMsfByteStream implementation. When we
  //     move to a proper interface implementation with shared storage state,
  //     this limitation will be removed.
  scoped_refptr<WritableMsfStream> writer2 = stream->GetWritableStream();
  EXPECT_EQ(writer1.get(), writer2.get());
}

TEST(WritableMsfByteStreamTest, WriterChangesReaderLengthButNotCursor) {
  scoped_refptr<MsfStream> reader(new MsfByteStream());
  scoped_refptr<WritableMsfStream> writer = reader->GetWritableStream();
  ASSERT_TRUE(writer.get() != NULL);

  EXPECT_EQ(reader->length(), 0u);
  EXPECT_EQ(reader->pos(), 0u);
  EXPECT_EQ(writer->length(), 0u);
  EXPECT_EQ(writer->pos(), 0u);
  writer->Consume(10);
  EXPECT_EQ(reader->length(), 10u);
  EXPECT_EQ(reader->pos(), 0u);
  EXPECT_EQ(writer->length(), 10u);
  EXPECT_EQ(writer->pos(), 10u);
}

}  // namespace msf