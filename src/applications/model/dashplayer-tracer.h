
#ifndef NDN_DASHPLAYER_TRACER_H
#define NDN_DASHPLAYER_TRACER_H

#include "ns3/node-container.h"
#include "ns3/callback.h"
#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/core-module.h"
#include "ns3/trace-source-accessor.h"

#include <boost/shared_ptr.hpp>


using namespace std;


namespace ns3 {

class Node;
class Packet;

/**
 * @ingroup ndn-tracers
 * @brief Tracer to obtain application-level delays
 */
class DASHPlayerTracer : public ns3::Object {
public:
  static void
  Destroy();

  /**
   * @brief Helper method to install tracers on all simulation nodes
   *
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This
   *tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   *
   */
  static void
  InstallAll(const std::string& file);

  /**
   * @brief Helper method to install tracers on the selected simulation nodes
   *
   * @param nodes Nodes on which to install tracer
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This
   *tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   *
   */
  static void
  Install(const NodeContainer& nodes, const std::string& file);

  /**
   * @brief Helper method to install tracers on a specific simulation node
   *
   * @param nodes Nodes on which to install tracer
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   * @param averagingPeriod How often data will be written into the trace file (default, every half
   *second)
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This
   *tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   *
   */
  static void
  Install(Ptr<Node> node, const std::string& file);

  /**
   * @brief Helper method to install tracers on a specific simulation node
   *
   * @param nodes Nodes on which to install tracer
   * @param outputStream Smart pointer to a stream
   * @param averagingPeriod How often data will be written into the trace file (default, every half
   *second)
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This
   *tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   */
  static Ptr<DASHPlayerTracer>
  Install(Ptr<Node> node, boost::shared_ptr<std::ofstream> os);

  /**
   * @brief Trace constructor that attaches to all applications on the node using node's pointer
   * @param os    reference to the output stream
   * @param node  pointer to the node
   */
  DASHPlayerTracer(boost::shared_ptr<std::ofstream> os, Ptr<Node> node);

  /**
   * @brief Trace constructor that attaches to all applications on the node using node's name
   * @param os        reference to the output stream
   * @param nodeName  name of the node registered using Names::Add
   */
  DASHPlayerTracer(boost::shared_ptr<std::ofstream> os, const std::string& node);

  /**
   * @brief Destructor
   */
  ~DASHPlayerTracer();

  /**
   * @brief Print head of the trace (e.g., for post-processing)
   *
   * @param os reference to output stream
   */
  void
  PrintHeader(std::ofstream& os) const;

private:
  void
  Connect();

  void
  ConsumeStats(Ptr<ns3::Application> app, unsigned int userId,
                               unsigned int segmentNr, std::string representationId,
                               unsigned int segmentExperiencedBitrate,
                               unsigned int stallingTime, unsigned int bufferLevel, std::vector<std::string> dependencyIds);

private:
  std::string m_node;
  Ptr<Node> m_nodePtr;

  boost::shared_ptr<std::ofstream> m_os;

};

} // namespace ns3

#endif
