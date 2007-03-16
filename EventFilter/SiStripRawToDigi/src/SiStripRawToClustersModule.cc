#include "EventFilter/SiStripRawToDigi/interface/SiStripRawToClustersModule.h"
#include "EventFilter/SiStripRawToDigi/interface/SiStripRawToDigiUnpacker.h"

//FWCore
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

//Data Formats
#include "DataFormats/SiStripCommon/interface/SiStripConstants.h"
#include "DataFormats/SiStripDigi/interface/SiStripDigiCollection.h"
#include "DataFormats/SiStripDigi/interface/SiStripDigi.h"
#include "DataFormats/SiStripDigi/interface/SiStripRawDigi.h"
#include "DataFormats/SiStripCluster/interface/SiStripCluster.h"
#include "DataFormats/SiStripCommon/interface/SiStripFedKey.h"

#include "CondFormats/SiStripObjects/interface/SiStripFedCabling.h"
#include "CondFormats/DataRecord/interface/SiStripFedCablingRcd.h"
#include "CalibFormats/SiStripObjects/interface/SiStripDetCabling.h"

#include "interface/shared/include/fed_header.h"
#include "interface/shared/include/fed_trailer.h"

#include "boost/cstdint.hpp"
#include <cstdlib>
#include <iostream> 
#include <memory>


using namespace std;
using namespace sistrip;

// -----------------------------------------------------------------------------
//
SiStripRawToClustersModule::SiStripRawToClustersModule( const edm::ParameterSet& conf ) :

  rawToDigi_(0),
  clusterizer_(0),
  fedCabling_(),
  detCabling_(0),
  productLabel_(conf.getUntrackedParameter<string>("ProductLabel","source")),
  productInstance_(conf.getUntrackedParameter<string>("ProductInstance","")),
  headerBytes_(conf.getUntrackedParameter<int>("AppendedBytes",0)),
  dumpFrequency_(conf.getUntrackedParameter<int>("FedBufferDumpFreq",0)),
  triggerFedId_(conf.getUntrackedParameter<int>("TriggerFedId",0)),
  useFedKey_(conf.getUntrackedParameter<bool>("UseFedKey",false)),
  clusters_(),
  fedEvents_()
  
{

  LogTrace(mlRawToDigi_)
    << "[SiStripRawToClustersModule::" << __func__ << "]"
    << " Constructing object...";
  
  clusterizer_ = new SiStripClusterizerFactory(conf);

 //Raw to digi
  
  rawToDigi_ = new SiStripRawToDigiUnpacker( headerBytes_, 
					     dumpFrequency_,
					     0,//FedEvent dump freq
					     triggerFedId_,
					     useFedKey_ );

  //Prepare event

  //produces< SiStripEventSummary >();
  produces< edm::DetSetVector<SiStripCluster> > (); 
}

// -----------------------------------------------------------------------------
/** */
SiStripRawToClustersModule::~SiStripRawToClustersModule() {
  

  if ( clusterizer_ ) 
    delete clusterizer_;
  if ( rawToDigi_ ) 
    delete rawToDigi_;

  LogTrace(mlRawToDigi_)
    << "[SiStripRawToClustersModule::" 
    << __func__ 
    << "]"
    << " Destructing object...";
}

// -----------------------------------------------------------------------------
void SiStripRawToClustersModule::beginJob( const edm::EventSetup& setup) {

  LogTrace(mlRawToDigi_) 
    << "[SiStripRawToClustersModule::"
    << __func__ 
    << "]";
  
  //Configure clusterizer factory
    clusterizer_->eventSetup(setup);

  //Fill cabling
  setup.get<SiStripFedCablingRcd>().get(fedCabling_);
  detCabling_ = new SiStripDetCabling(*fedCabling_.product());
  
  //reserve clusters container
  clusters_.reserve(detCabling_->getDetCabling().size());
  
  //Prepare Fed9UEvent cache
  fedEvents_.assign(1024,static_cast<Fed9U::Fed9UEvent*>(0));
}

// -----------------------------------------------------------------------------
void SiStripRawToClustersModule::endJob() {
  if (detCabling_) delete detCabling_;
}

// -----------------------------------------------------------------------------
/** 
*/
void SiStripRawToClustersModule::produce( edm::Event& event, 
					  const edm::EventSetup& setup ) {

  /*
  LogTrace(mlRawToDigi_) 
    << "[SiStripRawToDigiModule::" 
    << __func__ 
    << "]"
    << " Analyzing run/event "
    << event.id().run() << "/"
    << event.id().event();
  */

  // Retrieve FED raw data (by label, which is "source" by default)
  edm::Handle<FEDRawDataCollection> buffers;
  event.getByLabel( productLabel_, productInstance_, buffers ); 
  
  /*
  // Check if FEDs found in cabling map and event data
  if ( !fedCabling_->feds().empty() ) {
  LogTrace(mlRawToDigi_)
  << "[SiStripRawToClustersModule::" << __func__ << "]"
  << " Found " << fedCabling_->feds().size() 
  << " FEDs in cabling map!";
  } else {
  edm::LogWarning(mlRawToDigi_)
  << "[SiStripRawToClustersModule::" << __func__ << "]"
  << " No FEDs found in cabling map!";
  // Check which FED ids have non-zero size buffers
  pair<int,int> fed_range = FEDNumbering::getSiStripFEDIds();
  vector<uint16_t> feds;
  for ( uint16_t ifed = static_cast<uint16_t>(fed_range.first);
  ifed < static_cast<uint16_t>(fed_range.second); ifed++ ) {
  if ( ifed != triggerFedId_ && 
  buffers->FEDData( static_cast<int>(ifed) ).size() ) {
  feds.push_back(ifed);
  }
  }
  LogTrace(mlRawToDigi_)
  << "[SiStripRawToClustersModule::" << __func__ << "]"
  << " Found " << feds.size() << " FED buffers with non-zero size!";
  }
  */
  
  /*
  // Populate SiStripEventSummary object with "trigger FED" info
  auto_ptr<SiStripEventSummary> summary( new SiStripEventSummary() );
  rawToDigi_->triggerFed( *buffers, *summary ); 
  */
  
  //Iterate through det-ids
    map< uint32_t, vector<FedChannelConnection> >::const_iterator idet = detCabling_->getDetCabling().begin();
    for (; idet != detCabling_->getDetCabling().end(); idet++) {

      //If key is null continue;
      if ( !(idet->first) ) { continue; }
    
      //Calculate "fed-index" or "det-id" for DetSet id. ??
      uint32_t index = idet->first;//(idet->second[0].fedId() * 96) + idet->second[0].fedCh();

      //Add new DetSet to collection 
      edm::DetSet<SiStripCluster>& zs = *clusters_.insert(clusters_.end(),edm::DetSet<SiStripCluster>(index));
      zs.data.reserve(768); 

      //Loop over apv-pairs of det (ipair)
      for (uint16_t ipair = 0; ipair < idet->second.size(); ipair++) {
	
	//Get FedChannelConnection
	const FedChannelConnection& conn = idet->second[ipair];
	
	uint16_t fedId = conn.fedId();
	
	//If Fed hasnt already been initialised, extract data and initialise
	if (!fedEvents_[fedId]) {
	  
	  // Retrieve FED raw data for given FED 
	  const FEDRawData& input = buffers->FEDData( static_cast<int>(fedId) );
	  /*
	  // Locate start of FED buffer within raw data
	  FEDRawData output; 
	  rawToDigi_->locateStartOfFedBuffer( fedId, input, output );
	  */
	  
	  // Recast data to suit Fed9UEvent
	  Fed9U::u32* data_u32 = reinterpret_cast<Fed9U::u32*>( const_cast<unsigned char*>( input.data() ) );
	  Fed9U::u32  size_u32 = static_cast<Fed9U::u32>( input.size() / 4 ); 
	   
	  // Check on FEDRawData pointer
	  if ( !data_u32 ) {
	  edm::LogWarning(mlRawToDigi_)
	  << "[SiStripRawToDigiUnpacker::" << __func__ << "]"
	  << " NULL pointer to FEDRawData for FED id " << fedId;
	  continue;
	  }	
   
	  // Check on FEDRawData size
	  if ( !size_u32 ) {
	  edm::LogWarning(mlRawToDigi_)
	  << "[SiStripRawToDigiUnpacker::" << __func__ << "]"
	  << " FEDRawData has zero size for FED id " << fedId;
	  continue;
	  }

	  // Construct Fed9UEvent using present FED buffer
	  try {
	    fedEvents_[fedId] = new Fed9U::Fed9UEvent(data_u32,0,size_u32);
	  } catch(...) { rawToDigi_->handleException( __func__, "Problem when constructing Fed9UEvent" ); }

	  /*
	  //Check Fed9UEvent
	  try {
	  //fedEvents_[fedId]->checkEvent(); 
	  } catch(...) { rawToDigi_->handleException( __func__, "Problem when checking Fed9UEventStreamLine" ); }
	  */
	  
	  /*
	  // Retrieve readout mode
	  sistrip::FedReadoutMode mode = sistrip::UNDEFINED_FED_READOUT_MODE;
	  try {
	  mode = rawToDigi_->fedReadoutMode( static_cast<unsigned int>( fedEvents_[fedId]->getSpecialTrackerEventType() ) );
	  } catch(...) { rawToDigi_->handleException( __func__, "Problem extracting readout mode from Fed9UEvent" ); } 
	  
	  if ( mode != sistrip::ZERO_SUPPR ) { 
	  edm::LogWarning(mlRawToDigi_)
	  << "[SiStripRawClustersModule::" << __func__ << "]"
	  << " Readout mode for FED id " << fedId
	  << " not zero suppressed.";
	  continue;
	  }
	  */
	  
	  /*
	  // Dump of FEDRawData to stdout
	  if ( dumpFrequency_ && !(event.id().event()%dumpFrequency_) ) {
	  stringstream ss;
	  rawToDigi_->dumpRawData( fed_id, input, ss );
	  LogTrace(mlRawToDigi_) << ss.str();
	  }
	  */
	  
	}
	
	//Calculate corresponding FED unit, channel
	Fed9U::Fed9UAddress addr;
	uint16_t iunit = 0;
	uint16_t ichan = 0;
	uint16_t chan = 0;
	try {
	  addr.setFedChannel( static_cast<unsigned char>( conn.fedCh() ) );
	  //0-7 (internal)
	  iunit = addr.getFedFeUnit();/*getExternalFedFeUnit() for StreamLine*/
	  //0-11 (internal)
	  ichan = addr.getFeUnitChannel();/*getExternalFeUnitChannel()*/
	  //0-95 (internal)
	  chan = 12*( iunit ) + ichan;
	} catch(...) { 
	  rawToDigi_->handleException( __func__, "Problem using Fed9UAddress" ); 
	} 
	
	try{ 
	  
	  Fed9U::Fed9UEventIterator fed_iter = const_cast<Fed9U::Fed9UEventChannel&>(fedEvents_[fedId]->channel( iunit, ichan )).getIterator();
	  
	  for (Fed9U::Fed9UEventIterator i = fed_iter+7; i.size() > 0;) {
	    uint16_t strip = ipair*256 + *i++;
	    unsigned char width = *i++;       // cluster width in strips 
	    for ( uint16_t istr = 0; istr < ((uint16_t)width); istr++) {
	      clusterizer_->algorithm()->add(zs,(uint16_t)(strip+istr),(uint16_t)(*i++));
	    }
	  }
	  
	} catch(...) { 
	  stringstream sss;
	  sss << "Problem accessing ZERO_SUPPR data for FED id/ch: " 
	      << fedId << "/" << chan;
	  rawToDigi_->handleException( __func__, sss.str() ); 
	} 
      }
      clusterizer_->algorithm()->endDet(zs);
    }
  
  auto_ptr< edm::DetSetVector<SiStripCluster> > dsvclusters( new edm::DetSetVector<SiStripCluster>(clusters_) );

  // Write output to file
  //event.put(summary);
  event.put(dsvclusters);
  
  //reset fedEvents_ cache
  vector< Fed9U::Fed9UEvent*>::iterator ifedevent = fedEvents_.begin();
  for (; ifedevent!=fedEvents_.end(); ifedevent++) {
    if (*ifedevent) {
      delete (*ifedevent);
      *ifedevent=0;
    }
  }

  //reset clusters container
  clusters_.clear();
}
  

