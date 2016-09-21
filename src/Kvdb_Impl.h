/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _KV_DB_KVDB_IMPL_H_
#define _KV_DB_KVDB_IMPL_H_

#include <list>
#include <atomic>

#include "Db_Structure.h"
#include "BlockDevice.h"
#include "SuperBlockManager.h"
#include "IndexManager.h"
#include "DataHandle.h"
#include "SegmentManager.h"

using namespace std;

namespace kvdb {

    class KvdbDS {
    public:
        static KvdbDS* Create_KvdbDS(const char* filename,
                                    uint32_t hash_table_size,
                                    uint32_t segment_size);
        static KvdbDS* Open_KvdbDS(const char* filename);

        bool Insert(const char* key, uint32_t key_len,
                    const char* data, uint16_t length);
        bool Get(const char* key, uint32_t key_len, string &data);
        bool Delete(const char* key, uint32_t key_len);

        virtual ~KvdbDS();

    private:
        KvdbDS(const string& filename);
        bool openDB();
        bool closeDB();
        bool writeMetaDataToDevice();
        bool readMetaDataFromDevice();
        void startThds();
        void stopThds();

        bool insertKey(KVSlice& slice, OpType op_type);
        void updateIndex(Request *req);

        bool readData(HashEntry* entry, string &data);
        bool enqueReqs(Request *req);
        bool findAndLockSeg(Request *req, SegmentSlice*& seg_ptr);


    private:
        SuperBlockManager* sbMgr_;
        IndexManager* idxMgr_;
        BlockDevice* bdev_;
        SegmentManager* segMgr_;
        string fileName_;

    // Seg Write to device thread
    private:
        friend class SegWriteThd;
        class SegWriteThd : public Thread
        {
        public:
            SegWriteThd(): db_(NULL){}
            SegWriteThd(KvdbDS* db): db_(db){}
            virtual ~SegWriteThd(){}
            SegWriteThd(SegWriteThd& toBeCopied) = delete;
            SegWriteThd& operator=(SegWriteThd& toBeCopied) = delete;

            virtual void* Entry() { db_->SegWriteThdEntry(); return 0; }

        private:
            friend class KvdbDS;
            KvdbDS* db_;
        };
        SegWriteThd segWriteT_;
        std::list<SegmentSlice*> segWriteQue_;
        std::atomic<bool> segWriteT_stop_;
        Mutex segWriteQueMtx_;
        Cond segWriteQueCond_;

        void SegWriteThdEntry();


    private:
        static KvdbDS *instance_;

    };

}  // namespace kvdb

#endif  // #ifndef _KV_DB_KVDB_IMPL_H_