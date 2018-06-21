// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
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

#ifndef CONFIG_H
#define CONFIG_H

#define TABLE_CONTENT_MAX 256

#define HASH_TABLE_ENTRIES 1024
#define HASH_TABLE_BUCKET_ENTRIES 4

#define RX_QUEUE_LENGTH 256
#define TX_QUEUE_LENGTH 128

#define RX_FRAME_SIZE 2048
#define TX_FRAME_SIZE 2048

#define RX_FRAME_HEADROOM 128

#define RX_BURST_MAX 16
#define TX_BURST_DRAIN_NS 100000

#endif
