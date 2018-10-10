// Template for a test suite.

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Asserter.h>


#include "Asserts.h"
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <memory>

#include <CRingBuffer.h>
#include <CRingStateChangeItem.h>
#include <CPhysicsEventItem.h>
#include <CAllButPredicate.h>
#include <CRingItemFactory.h>

#include <iostream>

class ringmergetests : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(ringmergetests);
  CPPUNIT_TEST(singlering);
  CPPUNIT_TEST(multiring1);
  CPPUNIT_TEST(multiring2);
  CPPUNIT_TEST_SUITE_END();


private:
  pid_t        m_subProcess;
  
private:
  void removeRing(const char* name);
  void startMerger(const char** commandLine);
public:
  void setUp() {
    
    
    m_subProcess = -1;
    
    // just in case there are lingering rings.
    
    tearDown();
    
    // Always make both ring buffers - and init the pid to -1.
    
    CRingBuffer::create("output");
    CRingBuffer::create("inring1");
    CRingBuffer::create("inring2");
  }
  void tearDown() {
    if (m_subProcess != -1) {
      int status;
      kill(m_subProcess, 9);
      waitpid(m_subProcess, &status, 0);
      m_subProcess = -1;
    }
    removeRing("output");
    removeRing("inring1");
    removeRing("inring2");
  }
protected:
  void singlering();
  void multiring1();
  void multiring2();
};

// Remove ring which may or may not be there in the first place.

void
ringmergetests::removeRing(const char* ringName)
{
  try {
    CRingBuffer::remove(ringName);
  } catch(...) {}
}
// Start the merger 

void
ringmergetests::startMerger(const char** args)
{
  int m_subProcess = fork();
  if (m_subProcess == 0) {
    execv(
      "./ringmerge", const_cast<char* const*>(args)
    );         // Run the merge program.
  }
}

CPPUNIT_TEST_SUITE_REGISTRATION(ringmergetests);


void ringmergetests::singlering() {
  CAllButPredicate all;
  const char* params[]= {
    "./ringmerge", "--output=output",
    "--input=tcp://localhost/inring1",
    0
  };
  
  startMerger(params);
  sleep(1);                         // Let it get started.
  
  // Connect to input ring as a producer and the output ring as a consumer.
  

  std::unique_ptr<CRingBuffer> inring(new CRingBuffer("inring1", CRingBuffer::producer));
  std::unique_ptr<CRingBuffer> outring(new CRingBuffer("output"));
  
  // Make a begin run item, put it in the in ring and wait for it tomake it
  // to the outring.... then be sure we have the same data.

  CRingStateChangeItem item(BEGIN_RUN, 1, 0, 1234, "This is a title");
  item.commitToRing(*inring);
  
  // Wait for data in the output ring:
  
  while(! outring->availableData())
    ;
  std::unique_ptr<CRingItem> pItem(CRingItem::getFromRing(*outring, all));
  
  
  EQ(uint32_t(BEGIN_RUN), pItem->type());
  std::unique_ptr<CRingStateChangeItem>
    ps(static_cast<CRingStateChangeItem*>(CRingItemFactory::createRingItem(*pItem)));
  
  EQ(uint32_t(1), ps->getRunNumber());
  EQ(uint32_t(0), ps->getElapsedTime());
  EQ(uint32_t(1234), uint32_t(ps->getTimestamp()));
  EQ(std::string("This is a title"), ps->getTitle());
  

}
// Put a begin run item in each of two rings...differing source ids
// allow us to tell them apart.

void
ringmergetests::multiring1()
{
  CAllButPredicate all;
  const char* params[]= {
    "./ringmerge", "--output=output",
    "--input=tcp://localhost/inring1",
    "--input=tcp://localhost/inring2",
    0
  };
  
  startMerger(params);
  sleep(1);                         // Let it get started.
  
  // Connect to the three rings.
  
  std::unique_ptr<CRingBuffer> inring1(new CRingBuffer("inring1", CRingBuffer::producer));
  std::unique_ptr<CRingBuffer> inring2(new CRingBuffer("inring2", CRingBuffer::producer));
  std::unique_ptr<CRingBuffer> outring(new CRingBuffer("output"));

  // The pair of ring items we'll submit:
  
  CRingStateChangeItem begin(BEGIN_RUN, 1, 0, 1234, "This is a title");
  CRingStateChangeItem end(END_RUN, 1 ,0, 1234, "This is a title");
  
  // Commit the first and wait for it to get to the output ring.
  //  This gives us a deterministic order for the output items.
  
  begin.commitToRing(*inring1);
  while(!outring->availableData())
    ;
    
  // commit the second.
  
  end.commitToRing(*inring2);
  
  // First item out should be a begin run:
  
  std::unique_ptr<CRingItem> pItem1(CRingItem::getFromRing(*outring, all));
  
  EQ(uint32_t(BEGIN_RUN), pItem1->type());
  std::unique_ptr<CRingStateChangeItem>
    ps(static_cast<CRingStateChangeItem*>(CRingItemFactory::createRingItem(*pItem1)));
  
  EQ(uint32_t(1), ps->getRunNumber());
  EQ(uint32_t(0), ps->getElapsedTime());
  EQ(uint32_t(1234), uint32_t(ps->getTimestamp()));
  EQ(std::string("This is a title"), ps->getTitle());
  
  std::unique_ptr<CRingItem> pItem2(CRingItem::getFromRing(*outring, all));
  
  EQ(uint32_t(END_RUN), pItem2->type());
  std::unique_ptr<CRingStateChangeItem>
    pe(static_cast<CRingStateChangeItem*>(CRingItemFactory::createRingItem(*pItem2)));
  EQ(uint32_t(1), pe->getRunNumber());
  EQ(uint32_t(0), pe->getElapsedTime());
  EQ(uint32_t(1234), uint32_t(pe->getTimestamp()));
  EQ(std::string("This is a title"), pe->getTitle());


    
}

// Do a simple run with a few data events.  The begin/end items get put
// into inring1, the events into inring2:
//
//  - Before putting the events we ensure the output ring has gotten the
//  - begin.
//  - before putting the end we ensure the inring2 is empty.
//

void
ringmergetests::multiring2()
{
  CAllButPredicate all;
  const char* params[]= {
    "./ringmerge", "--output=output",
    "--input=tcp://localhost/inring1",
    "--input=tcp://localhost/inring2",
    0
  };
  
  startMerger(params);
  sleep(1);                         // Let it get started.
  
  // Connect to the three rings.
  
  std::unique_ptr<CRingBuffer> inring1(new CRingBuffer("inring1", CRingBuffer::producer));
  std::unique_ptr<CRingBuffer> inring2(new CRingBuffer("inring2", CRingBuffer::producer));
  std::unique_ptr<CRingBuffer> outring(new CRingBuffer("output"));

  // The  ring items we'll submit:
  
  CRingStateChangeItem begin(BEGIN_RUN, 1, 0, 1234, "This is a title");
  CRingStateChangeItem end(END_RUN, 1 ,0, 1234, "This is a title");
  CPhysicsEventItem item;
  uint16_t* p = static_cast<uint16_t*>(item.getBodyCursor());
  for (int i =0; i < 100; i++) {
    *p++ = i;                    // Counting pattern of data words.
  }
  item.setBodyCursor(p);
  item.updateSize();
  
  // Commit the first and wait for it to get to the output ring.
  //  This gives us a deterministic order for the output items.
  
  
  begin.commitToRing(*inring1);
  std::unique_ptr<CRingItem> pBegin(CRingItem::getFromRing(*outring, all));
  
  // We already know this is correct but...
  
  EQ(uint32_t(BEGIN_RUN), pBegin->type());

  // Commit a bunch of events...assuming the default ring item size is sufficient
  // for all of them to live in the rings:
  
  for (int i =0; i < 100; i++) {
    item.commitToRing(*inring2);
  }
  
  // Now get them all back out:
  
  for (int i = 0; i <100; i++) {
    std::unique_ptr<CRingItem> event(CRingItem::getFromRing(*outring, all));
    EQ(uint32_t(PHYSICS_EVENT), event->type());
    uint16_t* pBody = static_cast<uint16_t*>(event->getBodyPointer());
    for (int i =0; i < 100; i++) {
      EQ(uint16_t(i), *pBody);
      pBody++;
    }
  }
  
  // Put the end in and get it out:
  
  end.commitToRing(*inring1);
  std::unique_ptr<CRingItem> pEnd(CRingItem::getFromRing(*outring, all));
  EQ(uint32_t(END_RUN), pEnd->type());
}
