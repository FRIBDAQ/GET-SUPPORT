#include "GETDecoder.h"
GETDecoder* GETDecoder::m_pInstance = 0;

bool isSink = false;

static void
processRingItem(CRingItemProcessor& processor, CRingItem& item);   // Forward definition, see below.

static void
usage(std::ostream& o, const char* msg)
{
  o << msg << std::endl;
  o << "I think you forgot to load the file....";
  std::exit(EXIT_FAILURE);
}

GETDecoder::GETDecoder():
  m_sourceUrl(""), m_sinkUrl("tcp:///tmp/snapshot.evt")
{
}

GETDecoder::~GETDecoder()
{}

GETDecoder*
GETDecoder::getInstance()
{
  if(!m_pInstance) {
    m_pInstance = new GETDecoder();
  }
  // Regardless return it to the caller.
  return m_pInstance;

}

void
GETDecoder::SetUrls(std::string src){

  m_sourceUrl = src;
  std::cout << src << std::endl;
  
  std::vector<std::uint16_t> sample;     // Insert the sampled types here.
  std::vector<std::uint16_t> exclude;    // Insert the skippable types here.
  try {
    m_pDataSource =
      CDataSourceFactory::makeSource(m_sourceUrl, sample, exclude);
  }
  catch (CException& e) {
    usage(std::cerr, "Failed to open ring source");
  }
}

std::string
GETDecoder::SetDataSink(std::string src, std::string output, int nritems = 100){

  isSink = true;
  m_sourceUrl = src;
  m_sinkUrl = output;  
  
  // Create data source
  std::vector<std::uint16_t> sample;     // Insert the sampled types here.
  std::vector<std::uint16_t> exclude;    // Insert the skippable types here.
  try {
    m_pDataSource =
      CDataSourceFactory::makeSource(src, sample, exclude);
  }
  catch (CException& e) {
    usage(std::cerr, "Failed to open ring source");
  }
  
  // Create file data sink
  try {
    CDataSinkFactory factory;
    m_pDataSink = factory.makeSink(output);
  }
  catch (CException& e) {
    std::cerr << "Failed to create data sink: ";
    usage(std::cerr, e.ReasonText());
  }

  int counter = 0;
  CRingItem*  pItem;
  while ((pItem = m_pDataSource->getItem() )) {
    std::unique_ptr<CRingItem> item(pItem);     // Ensures deletion.
    m_pDataSink->putItem(*item);
    counter++;
    if (counter == nritems)
      break;
  }
  counter = 0;

  // Detach from online
  delete m_pDataSource;  
  // The new data source is the file data sink
  SetUrls(output);
  
  return output;
}

std::string
GETDecoder::GetSourceUrl()
{
  return m_sourceUrl;
}

std::string
GETDecoder::GetSinkUrl()
{
  return m_sinkUrl;
}

std::vector<NSCLGET::Hit>
GETDecoder::GetFrame()
{
  CRingItem*  pItem;
  CRingItemProcessor processor;

  pItem = m_pDataSource->getItem();
  std::unique_ptr<CRingItem> item(pItem);     // Ensures deletion.
  processRingItem(processor, *item);  

  return GetHits();
}

void
GETDecoder::SetHits(std::vector<NSCLGET::Hit>& hit)
{
  m_pHits = hit;
}

std::vector<NSCLGET::Hit>
GETDecoder::GetHits()
{
  return m_pHits;
}


/**
 * processRingItem.
 *    Modify this to put whatever ring item processing you want.
 *    In this case, we just output a message indicating when we have a physics
 *    event.  You  might replace this with code that decodes the body of the
 *    ring item and, e.g., generates appropriate root trees.
 *
 *  @param processor - references the ring item processor that handles ringitems
 *  @param item - references the ring item we got.
 */
static void
processRingItem(CRingItemProcessor& processor, CRingItem& item)
{
  // Create a dynamic ring item that can be dynamic cast to a specific one:

  CRingItem* castableItem = CRingItemFactory::createRingItem(item);
  std::unique_ptr<CRingItem> autoDeletedItem(castableItem);

  // Depending on the ring item type dynamic_cast the ring item to the
  // appropriate final class and invoke the correct handler.
  // the default case just invokes the unknown item type handler.

  switch (castableItem->type()) {
  case PERIODIC_SCALERS:
    {
      CRingScalerItem& scaler(dynamic_cast<CRingScalerItem&>(*castableItem));
      processor.processScalerItem(scaler);
      break;
    }
  case BEGIN_RUN:              // All of these are state changes:
  case END_RUN:
  case PAUSE_RUN:
  case RESUME_RUN:
    {
      CRingStateChangeItem& statechange(dynamic_cast<CRingStateChangeItem&>(*castableItem));
      processor.processStateChangeItem(statechange);
      break;
    }
  case PACKET_TYPES:                   // Both are textual item types
  case MONITORED_VARIABLES:
    {
      CRingTextItem& text(dynamic_cast<CRingTextItem&>(*castableItem));
      processor.processTextItem(text);
      break;
    }
  case PHYSICS_EVENT:
    {
      CPhysicsEventItem& event(dynamic_cast<CPhysicsEventItem&>(*castableItem));
      processor.processEvent(event);
      break;
    }
  case PHYSICS_EVENT_COUNT:
    {
      CRingPhysicsEventCountItem&
	eventcount(dynamic_cast<CRingPhysicsEventCountItem&>(*castableItem));
      processor.processEventCount(eventcount);
      break;
    }
  case RING_FORMAT:
    {
      CDataFormatItem& format(dynamic_cast<CDataFormatItem&>(*castableItem));
      processor.processFormat(format);
      break;
    }
  case EVB_GLOM_INFO:
    {
      CGlomParameters& glomparams(dynamic_cast<CGlomParameters&>(*castableItem));
      processor.processGlomParams(glomparams);
      break;
    }
  default:
    {
      processor.processUnknownItemType(item);
      break;
    }
  }
}

