#include <cassert>
#include <iostream>
#include <iterator>

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/slice.h"

int main() {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
  assert(status.ok());

  leveldb::WriteOptions write_options;
  leveldb::ReadOptions read_options;
  leveldb::Slice key{"1"};
  leveldb::Slice value{"666"};
  db->Put(write_options, key, value);
  std::string result;
  db->Get(read_options, key, &result);

  std::cout << "key: " << key.data() << ", value: " << result << std::endl;

  // iterator
  leveldb::Iterator* it = db->NewIterator(read_options);
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    std::cout << it->key().ToString() << ": " << it->value().ToString()
              << std::endl;
  }
  assert(it->status().ok());  // Check for any errors found during the scan
  delete it;

  return 0;
}
