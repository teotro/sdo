/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MEM_RUBY_SYSTEM_SEQUENCER_HH__
#define __MEM_RUBY_SYSTEM_SEQUENCER_HH__

#include <iostream>
#include <unordered_map>
#include <queue>

#include "mem/protocol/MachineType.hh"
#include "mem/protocol/RubyRequestType.hh"
#include "mem/protocol/SequencerRequestType.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/structures/CacheMemory.hh"
#include "mem/ruby/system/RubyPort.hh"
#include "params/RubySequencer.hh"

struct SequencerRequest
{
    PacketPtr pkt;
    RubyRequestType m_type;
    Cycles issue_time;
    // [Jiyong, MLDOM] we use this to buffer aliased exposures
    std::vector<PacketPtr> dependentRequests;

    SequencerRequest(PacketPtr _pkt, RubyRequestType _m_type,
                     Cycles _issue_time)
        : pkt(_pkt), m_type(_m_type), issue_time(_issue_time)
    {}
};

std::ostream& operator<<(std::ostream& out, const SequencerRequest& obj);

// Jiyong, MLDOM: remove SpecBuffer related code
struct SBB // SpecBufferBlock for each packet
{
  Addr reqAddress;
  unsigned reqSize;
  DataBlock data;
  bool data_hit;
};

// Jiyong: MLDOM: remove SpecBuffer related code
struct SBE // SpecBufferEntry for each load
{
  bool isSplit;
  SBB blocks[2];
};

class Sequencer : public RubyPort
{
  public:
    typedef RubySequencerParams Params;
    Sequencer(const Params *);
    ~Sequencer();

    // Public Methods
    void wakeup(); // Used only for deadlock detection
    void resetStats();
    void collateStats();
    void regStats();

    void writeCallback(Addr address,
                       DataBlock& data,
                       const bool externalHit = false,
                       const bool hitAtL0 = false,
                       const bool hitAtL1 = false,
                       const bool hitAtL2 = false,
                       const bool hitAtMem = false,
                       const MachineType mach = MachineType_NUM,
                       const Cycles initialRequestTime = Cycles(0),
                       const Cycles forwardRequestTime = Cycles(0),
                       const Cycles firstResponseTime = Cycles(0));

    void readCallback(Addr address,
                      DataBlock& data,
                      const bool externalHit = false,
                      const bool hitAtL0 = false,
                      const bool hitAtL1 = false,
                      const bool hitAtL2 = false,
                      const bool hitAtMem = false,
                      const MachineType mach = MachineType_NUM,
                      const Cycles initialRequestTime = Cycles(0),
                      const Cycles forwardRequestTime = Cycles(0),
                      const Cycles firstResponseTime = Cycles(0));

    // Jiyong, MLDOM: add callback function for MLDOM spec loads
    void readCallbackObliv_fromL0(Addr address,
                             const bool hitAtL0,
                             DataBlock& data_from_L0,
                             const int  reqIdx,
                             const MachineType mach = MachineType_NUM,
                             const Cycles initialRequestTime = Cycles(0),
                             const Cycles forwardRequestTime = Cycles(0),
                             const Cycles firstResponseTime = Cycles(0));

    // Jiyong, MLDOM: add callback function for MLDOM spec loads
    void readCallbackObliv_fromL1(Addr address,
                             const bool hitAtL1,
                             DataBlock& data_from_L1,
                             const int  reqIdx,
                             const MachineType mach = MachineType_NUM,
                             const Cycles initialRequestTime = Cycles(0),
                             const Cycles forwardRequestTime = Cycles(0),
                             const Cycles firstResponseTime = Cycles(0));

    // Jiyong, MLDOM: add callback function for MLDOM spec loads
    void readCallbackObliv_fromL2(Addr address,
                             const bool hitAtL2,
                             DataBlock& data_from_L2,
                             const int  reqIdx,
                             const MachineType mach = MachineType_NUM,
                             const Cycles initialRequestTime = Cycles(0),
                             const Cycles forwardRequestTime = Cycles(0),
                             const Cycles firstResponseTime = Cycles(0));

    // Jiyong, MLDOM: add callback function for MLDOM spec loads
    void readCallbackObliv_fromMem(Addr address,
                             const bool hitAtMem,
                             DataBlock& data_from_Mem,
                             const int  reqIdx,
                             const MachineType mach = MachineType_NUM,
                             const Cycles initialRequestTime = Cycles(0),
                             const Cycles forwardRequestTime = Cycles(0),
                             const Cycles firstResponseTime = Cycles(0));

    void hitCallbackObliv(SequencerRequest* srequest, bool hit, DataBlock& data, int fromLevel,
                          bool llscSuccess,
                          const MachineType mach, const bool externalHit,
                          const Cycles initialRequestTime,
                          const Cycles forwardRequestTime,
                          const Cycles firstResponseTime);

    // Jiyong, MLDOM: should remove these since no specbuf hit in MLDOM
    void specBufHitCallback();
    bool updateSBB(PacketPtr pkt, DataBlock& data, Addr address, bool data_hit);

    RequestStatus makeRequest(PacketPtr pkt);
    bool empty() const;
    int outstandingCount() const { return m_outstanding_count; }

    bool isDeadlockEventScheduled() const
    { return deadlockCheckEvent.scheduled(); }

    void descheduleDeadlockEvent()
    { deschedule(deadlockCheckEvent); }

    void print(std::ostream& out) const;
    void checkCoherence(Addr address);

    void markRemoved();
    void evictionCallback(Addr address, bool external);
    void invalidateSC(Addr address);
    int coreId() const { return m_coreId; }

    void recordRequestType(SequencerRequestType requestType);
    Stats::Histogram& getOutstandReqHist() { return m_outstandReqHist; }

    Stats::Histogram& getLatencyHist() { return m_latencyHist; }
    Stats::Histogram& getTypeLatencyHist(uint32_t t)
    { return *m_typeLatencyHist[t]; }

    Stats::Histogram& getHitLatencyHist() { return m_hitLatencyHist; }
    Stats::Histogram& getHitTypeLatencyHist(uint32_t t)
    { return *m_hitTypeLatencyHist[t]; }

    Stats::Histogram& getHitMachLatencyHist(uint32_t t)
    { return *m_hitMachLatencyHist[t]; }

    Stats::Histogram& getHitTypeMachLatencyHist(uint32_t r, uint32_t t)
    { return *m_hitTypeMachLatencyHist[r][t]; }

    Stats::Histogram& getMissLatencyHist()
    { return m_missLatencyHist; }
    Stats::Histogram& getMissTypeLatencyHist(uint32_t t)
    { return *m_missTypeLatencyHist[t]; }

    Stats::Histogram& getMissMachLatencyHist(uint32_t t) const
    { return *m_missMachLatencyHist[t]; }

    Stats::Histogram&
    getMissTypeMachLatencyHist(uint32_t r, uint32_t t) const
    { return *m_missTypeMachLatencyHist[r][t]; }

    Stats::Histogram& getIssueToInitialDelayHist(uint32_t t) const
    { return *m_IssueToInitialDelayHist[t]; }

    Stats::Histogram&
    getInitialToForwardDelayHist(const MachineType t) const
    { return *m_InitialToForwardDelayHist[t]; }

    Stats::Histogram&
    getForwardRequestToFirstResponseHist(const MachineType t) const
    { return *m_ForwardToFirstResponseDelayHist[t]; }

    Stats::Histogram&
    getFirstResponseToCompletionDelayHist(const MachineType t) const
    { return *m_FirstResponseToCompletionDelayHist[t]; }

    Stats::Counter getIncompleteTimes(const MachineType t) const
    { return m_IncompleteTimes[t]; }

  private:
    void issueRequest(PacketPtr pkt, RubyRequestType type);

    void hitCallback(SequencerRequest* request, DataBlock& data,
                     bool llscSuccess,
                     const MachineType mach, const bool externalHit,
                     const Cycles initialRequestTime,
                     const Cycles forwardRequestTime,
                     const Cycles firstResponseTime);

    void recordMissLatency(const Cycles t, const RubyRequestType type,
                           const MachineType respondingMach,
                           bool isExternalHit, Cycles issuedTime,
                           Cycles initialRequestTime,
                           Cycles forwardRequestTime, Cycles firstResponseTime,
                           Cycles completionTime);

    RequestStatus insertRequest(PacketPtr pkt, RubyRequestType request_type);
    RequestStatus insertSpecldRequest(PacketPtr pkt, RubyRequestType request_type);
    bool handleLlsc(Addr address, SequencerRequest* request);

    // Private copy constructor and assignment operator
    Sequencer(const Sequencer& obj);
    Sequencer& operator=(const Sequencer& obj);

  private:
    int m_max_outstanding_requests;
    Cycles m_deadlock_threshold;

    CacheMemory* m_dataCache_ptr;
    CacheMemory* m_instCache_ptr;

    // The cache access latency for top-level caches (L0/L1). These are
    // currently assessed at the beginning of each memory access through the
    // sequencer.
    // TODO: Migrate these latencies into top-level cache controllers.
    Cycles m_data_cache_hit_latency;
    Cycles m_inst_cache_hit_latency;

    typedef std::unordered_map<Addr, SequencerRequest*> RequestTable;
    RequestTable m_writeRequestTable;
    RequestTable m_readRequestTable;

    // define spec request table for spec loads
    struct hash_pair {
        template <class T1, class T2>
        std::size_t operator() (const std::pair<T1, T2>& p) const {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            return hash1 ^ hash2;
        }
    };
    typedef std::pair<Addr, int> SpecldKeyType; // <line_addr, ld_idx>
    typedef std::unordered_map<SpecldKeyType, SequencerRequest*, hash_pair> SpecRequestTable;
    SpecRequestTable m_specldRequestTable;  // Jiyong, MLDOM: request table for all spec loads, shouldn't alias with normal load
    // Global outstanding request count, across all request tables
    int m_outstanding_count;
    bool m_deadlock_check_scheduled;

    //! Counters for recording aliasing information.
    Stats::Scalar m_store_waiting_on_load;
    Stats::Scalar m_store_waiting_on_store;
    Stats::Scalar m_load_waiting_on_store;
    Stats::Scalar m_load_waiting_on_load;

    Stats::Scalar numSpecBufferHit;
    Stats::Scalar numPotentialMissedSpecBufferHit;

    int m_coreId;

    bool m_runningGarnetStandalone;

    //! Histogram for number of outstanding requests per cycle.
    Stats::Histogram m_outstandReqHist;

    //! Histogram for holding latency profile of all requests.
    Stats::Histogram m_latencyHist;
    std::vector<Stats::Histogram *> m_typeLatencyHist;

    //! Histogram for holding latency profile of all requests that
    //! hit in the controller connected to this sequencer.
    Stats::Histogram m_hitLatencyHist;
    std::vector<Stats::Histogram *> m_hitTypeLatencyHist;

    //! Histograms for profiling the latencies for requests that
    //! did not required external messages.
    std::vector<Stats::Histogram *> m_hitMachLatencyHist;
    std::vector< std::vector<Stats::Histogram *> > m_hitTypeMachLatencyHist;

    //! Histogram for holding latency profile of all requests that
    //! miss in the controller connected to this sequencer.
    Stats::Histogram m_missLatencyHist;
    std::vector<Stats::Histogram *> m_missTypeLatencyHist;

    //! Histograms for profiling the latencies for requests that
    //! required external messages.
    std::vector<Stats::Histogram *> m_missMachLatencyHist;
    std::vector< std::vector<Stats::Histogram *> > m_missTypeMachLatencyHist;

    //! Histograms for recording the breakdown of miss latency
    std::vector<Stats::Histogram *> m_IssueToInitialDelayHist;
    std::vector<Stats::Histogram *> m_InitialToForwardDelayHist;
    std::vector<Stats::Histogram *> m_ForwardToFirstResponseDelayHist;
    std::vector<Stats::Histogram *> m_FirstResponseToCompletionDelayHist;
    std::vector<Stats::Counter> m_IncompleteTimes;

    EventFunctionWrapper deadlockCheckEvent;

    std::vector<SBE> m_specBuf; // Jiyong, MLDOM: should remove
    std::queue<std::pair<PacketPtr, Tick>> m_specRequestQueue;
    EventFunctionWrapper specBufHitEvent;    // Jiyong, MLDOM: should remove
};

inline std::ostream&
operator<<(std::ostream& out, const Sequencer& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif // __MEM_RUBY_SYSTEM_SEQUENCER_HH__