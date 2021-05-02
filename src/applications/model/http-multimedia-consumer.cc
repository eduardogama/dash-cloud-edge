/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 Christian Kreuzberger and Daniel Posch, Alpen-Adria-University
 * Klagenfurt
 *
 * This file is part of amus-httpSIM, based on httpSIM. See AUTHORS for complete list of
 * authors and contributors.
 *
 * amus-httpSIM and httpSIM are free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * amus-httpSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * amus-httpSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "http-multimedia-consumer.h"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/core-module.h"
#include "ns3/trace-source-accessor.h"



NS_LOG_COMPONENT_DEFINE("MultimediaConsumer");

using namespace dash::mpd;

namespace ns3 {
typedef MultimediaConsumer<HttpClientDashApplication> HTTPMultimediaConsumer;

NS_OBJECT_ENSURE_REGISTERED(HTTPMultimediaConsumer);


template <typename Parent> std::string MultimediaConsumer<Parent>::alphabet = std::string("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");

template<class Parent>
TypeId MultimediaConsumer<Parent>::GetTypeId(void)
{
  static TypeId tid =
    TypeId((super::GetTypeId ().GetName () + "::MultimediaConsumer").c_str())
      .SetGroupName("Applications")
      .template SetParent<super>()
      .template AddConstructor<MultimediaConsumer>()
      .template AddAttribute("MpdFileToRequest", "URI to the MPD File to Request", StringValue("/"),
                    MakeStringAccessor(&MultimediaConsumer<Parent>::m_mpdUrl), MakeStringChecker())
      .template AddAttribute("ScreenWidth", "Width of the screen", UintegerValue(1920),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_screenWidth), MakeUintegerChecker<uint32_t>())
      .template AddAttribute("ScreenHeight", "Height of the screen", UintegerValue(1080),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_screenHeight), MakeUintegerChecker<uint32_t>())
      .template AddAttribute("MaxBufferedSeconds", "Maximum amount of buffered seconds allowed", UintegerValue(30),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_maxBufferedSeconds), MakeUintegerChecker<uint32_t>())
      .template AddAttribute("DeviceType", "PC, Laptop, Tablet, Phone, Game Console", StringValue("PC"),
                    MakeStringAccessor(&MultimediaConsumer<Parent>::m_deviceType), MakeStringChecker())
      .template AddAttribute("AllowUpscale", "Define whether or not the client has capabilities to upscale content with lower resolutions", BooleanValue(true),
                    MakeBooleanAccessor (&MultimediaConsumer<Parent>::m_allowUpscale), MakeBooleanChecker ())
      .template AddAttribute("AllowDownscale", "Define whether or not the client has capabilities to downscale content with higher resolutions", BooleanValue(false),
                    MakeBooleanAccessor (&MultimediaConsumer<Parent>::m_allowDownscale), MakeBooleanChecker ())
      .template AddAttribute("AdaptationLogic", "Defines the adaptation logic to be used; ",
                          StringValue("dash::player::AlwaysLowestAdaptationLogic"),
                    MakeStringAccessor (&MultimediaConsumer<Parent>::m_adaptationLogicStr), MakeStringChecker ())
      .template AddAttribute("StartRepresentationId", """Defines the representation ID of the representation to start streaming; "
                          "can be either an ID from the MPD file or one of the following keywords: "
                          "lowest, auto (lowest = the lowest representation available, auto = use adaptation logic to decide)",
                          StringValue("auto"),
                    MakeStringAccessor (&MultimediaConsumer<Parent>::m_startRepresentationId), MakeStringChecker ())
      .template AddAttribute("TraceNotDownloadedSegments", "Defines wether to trace or not to trace not downloaded segments", BooleanValue(false),
                    MakeBooleanAccessor(&MultimediaConsumer<Parent>::traceNotDownloadedSegments), MakeBooleanChecker())
      .template AddAttribute("StartUpDelay", "Defines the time to wait before trying to start playback", DoubleValue(2.0),
                    MakeDoubleAccessor(&MultimediaConsumer<Parent>::startupDelay), MakeDoubleChecker<double>())
      .template AddAttribute("UserId", "The ID of this user (optional)", UintegerValue(0),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_userId), MakeUintegerChecker<uint32_t>())
      .AddTraceSource("PlayerTracer", "Trace Player consumes of multimedia data",
                      MakeTraceSourceAccessor(&MultimediaConsumer<Parent>::m_playerTracer), "bla")
                    ;

  return tid;
}

template<class Parent>
MultimediaConsumer<Parent>::MultimediaConsumer() : super()
{
  NS_LOG_FUNCTION_NOARGS();
  this->mpd = NULL;
  this->mPlayer = NULL;
}

template<class Parent>
MultimediaConsumer<Parent>::~MultimediaConsumer()
{
}

///////////////////////////////////////////////////
//             Application Methods               //
///////////////////////////////////////////////////

// Start Application - initialize variables etc...
template<class Parent>
void MultimediaConsumer<Parent>::StartApplication() // Called at time specified by Start
{
  ostringstream oss;
  Ptr<Node> node = super::GetNode();
  node->GetObject<Ipv4>()->GetAddress(1,0).GetBroadcast().Print(oss);

  super::strNodeIpv4 = oss.str();
  super::node_id = super::GetNode ()->GetId();

  NS_LOG_DEBUG("Client(" << super::node_id << "," << super::strNodeIpv4 << "): Starting Multimedia Consumer - Device Type: " << m_deviceType);
  NS_LOG_DEBUG("Client(" << super::node_id << "," << super::strNodeIpv4 << "): Screen Resolution: " << m_screenWidth << "x" << m_screenHeight);
  NS_LOG_DEBUG("Client(" << super::node_id << "," << super::strNodeIpv4 << "): MPD File: " << m_mpdUrl << ", SuperClass: " << super::GetTypeId ().GetName ());

  string delim = "http://";
  string mpd_request_name;

  if (m_mpdUrl.find(delim) == 0) {
    string new_url = m_mpdUrl.substr(7);
    int pos = new_url.find("/");

    string hostname = new_url.substr(0,pos);
    super::m_hostName = super::getServerTableList(super::strNodeIpv4);

    fprintf(stderr, "Client(%d,%s): Hostname = %s\n", super::node_id, super::strNodeIpv4.c_str(), super::m_hostName.c_str());
    fprintf(stderr, "Client(%d,%s): Old Hostname = %s new Hostname = %s\n", super::node_id, super::strNodeIpv4.c_str(), hostname.c_str(), super::m_hostName.c_str());
    if (super::m_hostName != hostname) {
      stringstream ssValue;
      ifstream inputFile("UsersConnection");

      string line;
      while (getline(inputFile, line)) {
        vector<string> values = super::split(line, " ");
        string strNodeId = to_string(super::node_id);

        if(strNodeId == values[0]) {
          ssValue << values[0] << " " << values[1] << " " <<  values[2] << " " << values[3] << " " << super::m_hostName << endl;
        } else {
          ssValue << values[0] << " " << values[1] << " " <<  values[2] << " " << values[3] << " " << values[4] << endl;
        }
      }
      inputFile.close();

      ofstream newOutputFile;
      newOutputFile.open("UsersConnection", ios::out);
      newOutputFile << ssValue.str();
      newOutputFile.flush();
      newOutputFile.close();
    }

    super::SetRemote(Ipv4Address(super::m_hostName.c_str()),80);
    mpd_request_name = new_url.substr(pos+1);
  }

  stringstream ss_tempDir;
  ss_tempDir << "-ns3-node-" << super::node_id;

  for (size_t i = 0; i < 8; i++) {
    ss_tempDir << MultimediaConsumer<Parent>::alphabet[rand()%MultimediaConsumer<Parent>::alphabet.size()];
  }

  this->m_tempDir = ns3::SystemPath::MakeTemporaryDirectoryName() + ss_tempDir.str();

  NS_LOG_DEBUG("Client(" << super::node_id << "): Temporary Directory: " << m_tempDir);
  ns3::SystemPath::MakeDirectories(this->m_tempDir);

  this->m_tempMpdFile = this->m_tempDir + "/mpd.xml.gz";

  super::m_isMpd = true;

  this->m_mpdParsed                = false;
  this->m_initSegmentIsGlobal      = false;
  this->m_hasInitSegment           = false;
  this->m_hasDownloadedAllSegments = false;
  this->m_freezeStartTime          = 0;
  this->totalConsumedSegments      = 0;
  this->requestedRepresentation    = NULL;
  this->requestedSegmentURL        = NULL;

  this->m_currentDownloadType = MPD;
  this->m_startTime = Simulator::Now().GetMilliSeconds();

  NS_LOG_DEBUG("Client(" << super::node_id << "): Trying to instantiate MultimediaPlayer(aLogic=" << this->m_adaptationLogicStr << ")");

  this->mPlayer = new dash::player::MultimediaPlayer(this->m_adaptationLogicStr, this->m_maxBufferedSeconds);

  NS_ASSERT_MSG(this->mPlayer->GetAdaptationLogic() != NULL, "Could not initialize adaptation logic...");

  super::SetAttribute("FileToRequest", StringValue(mpd_request_name));
  super::SetAttribute("WriteOutfile", StringValue(m_tempMpdFile));
  super::SetAttribute("KeepAlive", StringValue("true"));

  // do base stuff
  super::StartApplication();
  fprintf(stderr, "Client(%d): Done starting multimedia application!\n", super::node_id);
}

// Stop Application - Cancel any outstanding events
template<class Parent>
void MultimediaConsumer<Parent>::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // Cancelling Event Timers
  m_consumerLoopTimer.Cancel();
  Simulator::Cancel(m_consumerLoopTimer);

  m_downloadEventTimer.Cancel();
  Simulator::Cancel(m_downloadEventTimer);

  /*OK LOG ALL NOT RECEIVED FILES FROM MPD*/
  if (traceNotDownloadedSegments) {
    //check if mpd and player exists
    if (mpd != NULL && mPlayer != NULL) {
      //first consume everything from buffer
      while (consume() > 0.0);

      //ok check how many segments we have not consumed
      while (totalConsumedSegments < mPlayer->GetAdaptationLogic()->getTotalSegments()) {
        m_playerTracer(this, m_userId, totalConsumedSegments++,  "0", 0, 0, 0, std::vector<std::string>());
      }
    }
  }

  // clean up mpd/DASH specific stuff
  if (mpd != NULL) {
    delete mpd;
    mpd = NULL;
  }

  if (mPlayer != NULL) {
    delete mPlayer;
    mPlayer = NULL;
  }

  super::SetAttribute("KeepAlive", StringValue("false"));

  // make sure to close the socket, in case it is still open
  super::ForceCloseSocket();

  // cleanup base stuff
  super::StopApplication();
}

template<class Parent>
void MultimediaConsumer<Parent>::OnFileReceived(unsigned status, unsigned length)
{
  super::OnFileReceived(status, length);

  fprintf(stderr, "Client: On File Received called\n");
  if (!m_mpdParsed) {
    OnMpdFile();
  } else {
    OnMultimediaFile();
  }
}

template<class Parent>
void MultimediaConsumer<Parent>::OnMpdFile()
{
  // fprintf(stderr, "Client(%d): On MPD File...\n", super::node_id);

  // check if file was gziped, if not, we use it as is
  if (this->m_tempMpdFile.find(".gz") != std::string::npos)
  {
    // file was compressed, decompress it
    NS_LOG_DEBUG("GZIP MPD File " << m_tempMpdFile << " received. Decompressing...");
    string newFileName = this->m_tempMpdFile.substr(0, this->m_tempMpdFile.find(".gz"));

    // TODO DECOMPRESS
    if(DecompressFile(this->m_tempMpdFile, newFileName))
      this->m_tempMpdFile = newFileName;
  }

  dash::IDASHManager *manager;
  manager = CreateDashManager();
  this->mpd = manager->Open((char*)m_tempMpdFile.c_str());

  // We don't need the manager anymore...
  manager->Delete();
  manager = NULL;

  if (this->mpd == NULL) {
    NS_LOG_ERROR("Error parsing mpd " << m_tempMpdFile);
    return;
  }

  // we are assuming there is only 1 period, get the first one
  IPeriod *currentPeriod = mpd->GetPeriods().at(0);

  // get base URLs
  this->m_baseURL = "";
  vector<dash::mpd::IBaseUrl*> baseUrls = this->mpd->GetBaseUrls ();

  if (baseUrls.size() > 0) {
    if (baseUrls.size() == 1) {
      this->m_baseURL = baseUrls.at(0)->GetUrl();
    } else {
      int randUrl = rand() % baseUrls.size();
      NS_LOG_DEBUG("Mutliple base URLs available, selecting a random one... ");
      this->m_baseURL = baseUrls.at(randUrl)->GetUrl();
    }

    // m_baseURL needs to be parsed
    // starts with http://
    string delim = "http://";

    if (this->m_baseURL.find(delim) == 0) {
      string hostname = this->m_baseURL.substr(7);
      // get host
      this->m_baseURL = hostname.substr(hostname.find("/")+1);

      hostname = hostname.substr(0, hostname.find("/"));
      super::SetRemote(Ipv4Address(hostname.c_str()),80);
      NS_LOG_DEBUG("Client(" << super::node_id << "): Base URL: " << this->m_baseURL << ", hostname = " << hostname);
    } else {
      NS_LOG_ERROR("Client(" << super::node_id << "): Could not properly parse baseURL. Expected 'http://ipAddress/folder/blub/'.");
      return;
    }
  } else {
    NS_LOG_ERROR("Client(" << super::node_id << "): No Base URL provided in MPD file... exiting.");
    return;
  }

  // Get the adaptation sets, though we are only takeing the first one
  vector<IAdaptationSet *> allAdaptationSets = currentPeriod->GetAdaptationSets();

  // we are assuming that allAdaptationSets.size() == 1
  if (allAdaptationSets.size() == 0) {
    NS_LOG_ERROR("Client(" << super::node_id << "): No adaptation sets found in MPD file... exiting.");
    return;
  }

  // use first adaptation set
  IAdaptationSet* adaptationSet = allAdaptationSets.at(0);

  // check if the adaptation set has an init segment
  // alternatively, the init segment is representation-specific
  NS_LOG_DEBUG("Checking for init segment in adaptation set...");
  string initSegment = "";

  if (adaptationSet->GetSegmentBase () && adaptationSet->GetSegmentBase ()->GetInitialization ()) {
    NS_LOG_DEBUG("Adaptation Set has INIT Segment");
    // get URL to init segment
    initSegment = adaptationSet->GetSegmentBase ()->GetInitialization ()->GetSourceURL ();
    // TODO: request init segment
    this->m_initSegmentIsGlobal = true;
    this->m_hasInitSegment = true;
  } else {
    NS_LOG_DEBUG("Adaptation Set does not have INIT Segment");
    this->m_hasInitSegment = false;
  }

  vector<IRepresentation*> reps = adaptationSet->GetRepresentation();

  NS_LOG_DEBUG("Client(" << super::node_id << "): MPD file contains " << reps.size() << " Representations: ");
  NS_LOG_DEBUG("Client(" << super::node_id << "): Start Representation: " << this->m_startRepresentationId);

  // calculate segment duration
  NS_LOG_DEBUG("Client(" << super::node_id << "): Period Duration:" << reps.at(0)->GetSegmentList()->GetDuration());

  bool startRepresentationSelected = false;

  string firstRepresentationId = "";
  string bestRepresentationBasedOnBandwidth = "";

  this->mPlayer->SetLastDownloadBitRate(super::lastDownloadBitrate);

  NS_LOG_DEBUG("Client(" << super::node_id << "): Download Speed of MPD file was : " << super::lastDownloadBitrate << " bits per second");
  this->m_isLayeredContent = false;

  this->m_availableRepresentations.clear();

  vector<IRepresentation* >::iterator it;
  //for (IRepresentation* rep : reps)
  for (it = reps.begin(); it != reps.end(); ++it)
  {
    IRepresentation* rep = *it;
    unsigned width = rep->GetWidth();
    unsigned height = rep->GetHeight();

    // if not allowed to upscale, skip this representation
    if (!this->m_allowUpscale && width < this->m_screenWidth && height < this->m_screenHeight) {
      continue;
    }

    // if not allowed to downscale and width/height are too large, skip this representation
    if (!this->m_allowDownscale && width > this->m_screenWidth && height > this->m_screenHeight) {
      continue;
    }

    string repId = rep->GetId();

    if (firstRepresentationId == "") {
      firstRepresentationId = repId;
    }

    vector<string> dependencies = rep->GetDependencyId ();
    unsigned requiredDownloadSpeed = rep->GetBandwidth();

    if (dependencies.size() > 0) // we found out that this is layered content
      this->m_isLayeredContent = true;

    NS_LOG_DEBUG("ID = " << repId << ", DepId=" << dependencies.size() << ", width=" << width << ", height=" << height << ", bwReq=" << requiredDownloadSpeed);

    if (!startRepresentationSelected && this->m_startRepresentationId != "lowest") {
      if (this->m_startRepresentationId == "auto") {
        // do we have enough bandwidth available?
        if (super::lastDownloadBitrate > requiredDownloadSpeed) {
          // yes we do!
          bestRepresentationBasedOnBandwidth = repId;
        }
      } else if (rep->GetId() == this->m_startRepresentationId) {
        NS_LOG_DEBUG("The last representation is the start representation!");
        startRepresentationSelected = true;
      }
    }

    this->m_availableRepresentations[repId] = rep;
  }

  // check m_startRepresentationId
  if (this->m_startRepresentationId == "lowest") {
    NS_LOG_DEBUG("Using Lowest available representation; ID = " << firstRepresentationId);
    this->m_startRepresentationId = firstRepresentationId;
    startRepresentationSelected = true;
  } else if (this->m_startRepresentationId == "auto") {
    // select representation based on bandwidth
    if (bestRepresentationBasedOnBandwidth != "") {
      NS_LOG_DEBUG("Using best representation based on bandwidth; ID = " << bestRepresentationBasedOnBandwidth);
      this->m_startRepresentationId = bestRepresentationBasedOnBandwidth;
      startRepresentationSelected = true;
    }
  }

  // was there a start representation selected?
  if (!startRepresentationSelected) {
    // IF NOT, default to lowest
    NS_LOG_DEBUG("No start representation selected, default to lowest available representation; ID = " << firstRepresentationId);
    this->m_startRepresentationId = firstRepresentationId;
    startRepresentationSelected = true;
  }

  this->m_curRepId = this->m_startRepresentationId;

  // okay, check init segment
  if (initSegment == "" && this->m_hasInitSegment == true) {
    NS_LOG_DEBUG("Using init segment of representation " << m_startRepresentationId);
    initSegment = this->m_availableRepresentations[this->m_startRepresentationId]->GetSegmentBase()->GetInitialization()->GetSourceURL();
    NS_LOG_DEBUG("Init Segment URL = " << initSegment);
  }

  this->m_mpdParsed = true;
  this->mPlayer->SetAvailableRepresentations(&this->m_availableRepresentations);

  // trigger MPD parsed after x seconds
  unsigned long curTime = Simulator::Now().GetMilliSeconds();
  NS_LOG_DEBUG("MPD received after " << (curTime - this->m_startTime) << " ms");

  if (initSegment == "") {
    NS_LOG_DEBUG("No init Segment selected.");
    // schedule streaming of first segment
    this->m_currentDownloadType = Segment;
    ScheduleDownloadOfSegment();
  } else {
    // Schedule streaming of init segment
    this->m_initSegment = initSegment;
    this->m_currentDownloadType = InitSegment;
    ScheduleDownloadOfInitSegment();
  }

  // we received the MDP, so we can now start the timer for playing
  SchedulePlay(startupDelay);

  /* also we can delete the folder (m_tempDir) the MPD is stored in */
  string rmdir_cmd = "rm -rf " + m_tempDir;
  if (system(rmdir_cmd.c_str()) != 0) {
    fprintf(stderr, "Error: could not delete directory '%s'.\n", m_tempDir.c_str());
  }

  super::m_isMpd = false;
}

template<class Parent>
void MultimediaConsumer<Parent>::OnMultimediaFile()
{
  fprintf(stderr, "Client(%d): On Multimedia File '%s'\n", super::node_id, super::m_fileToRequest.c_str());

  if (!super::m_active) {
    return;
  }

  // get the current representation id
  // and check if this was an init segment
  if (m_currentDownloadType == InitSegment) {
    // init segment
    if (m_initSegmentIsGlobal) {
      m_downloadedInitSegments.push_back("GlobalAdaptationSet");
      NS_LOG_DEBUG("Global Init Segment received");
      // cout << "Global Init Segment received" << '\n';
    } else {
      m_downloadedInitSegments.push_back(m_curRepId);
      NS_LOG_DEBUG("Init Segment received (rep=" << m_curRepId << ")");
      // cout << "Init Segment received (rep=" << m_curRepId << ")" << '\n';
    }
    // getchar();
  } else {
    // normal segment

    //fprintf(stderr, "lastBitrate = %f\n", super::lastDownloadBitrate);
    mPlayer->SetLastDownloadBitRate(super::lastDownloadBitrate);

    // fprintf(stderr, "Last Download Speed = %f kBit/s\n", super::lastDownloadBitrate/1000.0);

    // check if there is enough space in buffer
    if(mPlayer->EnoughSpaceInBuffer(requestedSegmentNr, requestedRepresentation, m_isLayeredContent)) {
      if(mPlayer->AddToBuffer(requestedSegmentNr, requestedRepresentation, super::lastDownloadBitrate, m_isLayeredContent))
        NS_LOG_DEBUG("Segment Accepted for Buffering");
      else
        NS_LOG_DEBUG("Segment Rejected for Buffering");
    } else {
      // try again in 1 second, and again and again... but do not donwload anything in the meantime
      Simulator::Schedule(Seconds(1.0), &MultimediaConsumer<Parent>::OnMultimediaFile, this);
      return;
    }
  }

  m_currentDownloadType = Segment;
  ScheduleDownloadOfSegment();
}

template<class Parent>
bool MultimediaConsumer<Parent>::DecompressFile(string source, string filename)
{
  NS_LOG_FUNCTION(source << filename);
  ifstream infile( source.c_str(), ios_base::in | ios_base::binary ); //Creates the input stream

  //Tests if the file is being opened correctly.
  if (!infile) {
   cerr << "Can't open file: " << source << endl;
   return false;
  }

  // read
  string compressed_str((istreambuf_iterator<char>(infile)), istreambuf_iterator<char>());

  try {
    string decompressed = zlib_decompress_string(compressed_str);

    ofstream outfile( filename.c_str(), ios_base::out |  ios_base::binary ); //Creates the output stream
    outfile << decompressed;
    outfile.close();
  } catch(std::exception& e) {
    NS_LOG_DEBUG(e.what() << " Assuming file was not zipped!");
    return false;
  }

  return true;
}

template<class Parent>
void MultimediaConsumer<Parent>::SchedulePlay(double wait_time)
{
  m_consumerLoopTimer.Cancel();
  m_consumerLoopTimer = Simulator::Schedule(Seconds(wait_time), &MultimediaConsumer<Parent>::DoPlay, this);
}

template<class Parent>
void MultimediaConsumer<Parent>::DoPlay()
{
  double consumed_sec = consume();

  if (consumed_sec > 0) {
    SchedulePlay(consumed_sec);
  } else if (consumed_sec == 0 && m_hasDownloadedAllSegments) {
    return;
  } else { // Stall event
    SchedulePlay(); //restart timer

    if (requestedRepresentation != NULL && !m_hasDownloadedAllSegments && requestedRepresentation->GetDependencyId().size() > 0) {
      if (!mPlayer->GetAdaptationLogic()->hasMinBufferLevel(requestedRepresentation)) { // check buffer state
        //abort download ...
        NS_LOG_DEBUG("Aborting to download a segment with repId = " << requestedRepresentation->GetId().c_str());
        super::StopApplication();
        mPlayer->SetLastDownloadBitRate(0.0);//set dl_bitrate to zero.
        ScheduleDownloadOfSegment();
      }
    }
  }
}

template<class Parent>
double MultimediaConsumer<Parent>::consume()
{
  unsigned buffer_level = this->mPlayer->GetBufferLevel();

  if (buffer_level == 0 && this->m_hasDownloadedAllSegments == true) {
    NS_LOG_DEBUG("Multimedia Streaminig  Finished (Cur Buffer Level = " << buffer_level << ")");
    return 0.0;
  }

  dash::player::MultimediaBuffer::BufferRepresentationEntry entry = this->mPlayer->ConsumeFromBuffer();
  double consumedSeconds = entry.segmentDuration;

  if (consumedSeconds > 0) {
    NS_LOG_DEBUG("Cur Buffer Level = " << buffer_level << ", Consumed Segment " << entry.segmentNumber << ", with Rep " << entry.repId << " for " << entry.segmentDuration << " seconds");
    int64_t freezeTime = 0;

    if (!this->m_hasStartedPlaying) {
      // we havent started yet, so we can measure the start-up delay until now
      this->m_hasStartedPlaying = true;
      int64_t startUpDelay = Simulator::Now().GetMilliSeconds() - this->m_startTime;
      // LOG STARTUP DELAY HERE
      freezeTime = startUpDelay;
      NS_LOG_DEBUG("Cur Buffer Level = " << buffer_level << ", started consuming ... (Start-Up Delay: " << startUpDelay << " milliseconds)");
    } else if (this->m_freezeStartTime != 0) {
      // we had a freeze/stall, but we can continue playing now
      // measure:
      freezeTime = (Simulator::Now().GetMilliSeconds() - m_freezeStartTime);
      this->m_freezeStartTime = 0;
      NS_LOG_DEBUG("Freeze Of " << freezeTime << " milliseconds is over!");
    }

    //fprintf(stderr,  "Current Buffer Level: %f\n", mPlayer->GetBufferLevel());
    m_playerTracer(this, this->m_userId, entry.segmentNumber, entry.repId, entry.experienced_bitrate_bit_s, freezeTime, (unsigned) (this->mPlayer->GetBufferLevel()), entry.depIds);

    this->totalConsumedSegments++;
    return consumedSeconds;
  } else {
    // could not consume, means buffer is empty
    if (this->m_freezeStartTime == 0 && this->m_hasStartedPlaying == true) {
      // this actually means that we have a stall/free (m_hasStartedPlaying == false would mean that this is part of start up delay)
      // set m_freezeStartTime
      this->m_freezeStartTime = Simulator::Now().GetMilliSeconds();
    }

    // continue trying to consume... - these are unsmooth seconds
    return 0.0; // default parameter
  }
}

template<class Parent>
void MultimediaConsumer<Parent>::ScheduleDownloadOfInitSegment()
{
  // wait some time (10 milliseconds) before requesting the next first segment
  // basically, we simulate that parsing the MPD takes 10 ms on the client this
  // assumption might not be true generally speaking, but not waiting at all
  // is worse.
  Simulator::Schedule(Seconds(0.01), &MultimediaConsumer<Parent>::DownloadInitSegment, this);
}

template<class Parent>
void MultimediaConsumer<Parent>::ScheduleDownloadOfSegment()
{
  // wait 1 ms (dummy time) before downloading next segment - this prevents some issues
  // with start/stop application and interests coming in late.
  this->m_downloadEventTimer.Cancel();
  this->m_downloadEventTimer = Simulator::Schedule(Seconds(0.001), &MultimediaConsumer<Parent>::DownloadSegment, this);
}

template<class Parent>
void MultimediaConsumer<Parent>::DownloadInitSegment()
{
  NS_LOG_DEBUG("Downloading init segment... " << m_baseURL + m_initSegment << ";");
  // getchar();
}

template<class Parent>
void MultimediaConsumer<Parent>::DownloadSegment()
{
  // get segment number and rep id
  this->requestedRepresentation = NULL;
  this->requestedSegmentNr = 0;

  this->requestedSegmentURL = this->mPlayer->GetAdaptationLogic()->GetNextSegment(&this->requestedSegmentNr, &this->requestedRepresentation, &this->m_hasDownloadedAllSegments);
  // fprintf(stderr, "Multimediaconsumer::Downloadsegment()\n");

  if(this->m_hasDownloadedAllSegments) { // DONE
    NS_LOG_DEBUG("No more segments available for download!\n");
    // make sure to close the socket
    super::ForceCloseSocket();
    return;
  }

  if (this->requestedSegmentURL == NULL) { //IDLE
    NS_LOG_DEBUG("IDLE\n");
    this->m_downloadEventTimer = Simulator::Schedule(Seconds(1.0), &MultimediaConsumer<Parent>::DownloadSegment, this);
    return;
  }

  super::StopApplication();
  super::SetAttribute("FileToRequest", StringValue(m_baseURL + requestedSegmentURL->GetMediaURI()));
  super::SetAttribute("WriteOutfile", StringValue(""));
  super::StartApplication();
}

} // namespace ns3
