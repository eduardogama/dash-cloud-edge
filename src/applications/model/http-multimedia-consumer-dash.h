#ifndef HTTP_MULTIMEDIACONSUMER_DASH_H
#define HTTP_MULTIMEDIACONSUMER_DASH_H

#include "http-client-dash.h"

#include "libdash.h" //libdash library
#include "multimedia-player.h" //libdash library


#define MULTIMEDIA_CONSUMER_LOOP_TIMER 0.1
#define MIN_BUFFER_LEVEL 4.0


using namespace dash::mpd;


namespace ns3 {

template<class Parent>
class MultimediaConsumerDash : public Parent
{
  typedef Parent super;

public:
  enum DownloadType { MPD = 0, InitSegment = 1, Segment = 2 };
  static TypeId
  GetTypeId();

  MultimediaConsumer();
  virtual ~MultimediaConsumer();

  virtual void StartApplication();
  virtual void StopApplication();

};

}


#endif //HTTP_MULTIMEDIACONSUMER_DASH_H
