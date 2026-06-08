//  NIST-developed software is provided by NIST as a public service. You may use, copy and distribute copies of the
//  software in any medium, provided that you keep intact this entire notice. You may improve, modify and create
//  derivative works of the software or any portion of the software, and you may copy and distribute such modifications
//  or works. Modified works should carry a notice stating that you changed the software and should note the date and
//  nature of any such change. Please explicitly acknowledge the National Institute of Standards and Technology as the
//  source of the software. NIST-developed software is expressly provided "AS IS." NIST MAKES NO WARRANTY OF ANY KIND,
//  EXPRESS, IMPLIED, IN FACT OR ARISING BY OPERATION OF LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR
//  WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE
//  CORRECTED. NIST DOES NOT WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE SOFTWARE OR THE RESULTS
//  THEREOF, INCLUDING BUT NOT LIMITED TO THE CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE. You
//  are solely responsible for determining the appropriateness of using and distributing the software and you assume
//  all risks associated with its use, including but not limited to the risks and costs of program errors, compliance
//  with applicable laws, damage to or loss of data, programs or equipment, and the unavailability or interruption of
//  operation. This software is not intended to be used in any situation where a failure could cause risk of injury or
//  damage to property. The software developed by NIST employees is not subject to copyright protection within the
//  United States.

#ifndef HEDGEHOG_STATE_MULTI_SENDERS_H_
#define HEDGEHOG_STATE_MULTI_SENDERS_H_

#include "state_sender.h"
#include "multi_senders.h"

/// @brief Hedgehog main namespace
namespace hh {
/// @brief Hedgehog behavior namespace
namespace behavior {

/// @brief Behavior abstraction for states that send multiple types of data
/// @tparam Outputs Types of data the state sends
template<class ...Outputs>
class StateMultiSenders : public MultiSenders<Outputs...>, public StateSender<Outputs>... {
 private:
  std::tuple<std::vector<std::shared_ptr<Outputs>>...> batchSendQueues_; ///< Vector for batch send (bufferResult/flushResults)

 public:
  /// @brief Default constructor
  StateMultiSenders() = default;

  /// @brief Default destructor
  ~StateMultiSenders()  override = default;

  /// @brief Add result to the ready list
  /// @tparam DataType Type of the data, should be part of the state Output types
  /// @param data Data of type DataType added to the ready list
  template<tool::MatchOutputTypeConcept<Outputs...> DataType>
  void addResult(std::shared_ptr<DataType> data) { StateSender<DataType>::readyList()->push(data); }

  /// @brief Add batch result to the ready list
  /// @tparam DataType Type of the data, should be part of the state Output types
  /// @param datas Datas of type DataType added to the ready list
  template<tool::MatchOutputTypeConcept<Outputs...> DataType>
  void batchAddResult(std::vector<std::shared_ptr<DataType>> const &datas) {
    for (auto data : datas) {
      StateSender<DataType>::readyList()->push(data);
    }
  }

  /// @brief Buffer a new result (do not send yet)
  /// @tparam DataType Type of the data, should be part of the state Output types
  /// @param data Data of type DataType that is buffered
  template<tool::MatchOutputTypeConcept<Outputs...> DataType>
  void bufferResult(std::shared_ptr<DataType> data) {
    std::get<std::vector<std::shared_ptr<DataType>>>(this->batchSendQueues_).emplace_back(data);
  }

  /// @brief Send all the buffered data of a specific type.
  /// @tparam DataType Type of the data that is sent, should be part of the state Output types
  template<tool::MatchOutputTypeConcept<Outputs...> DataType>
  void flushResults() {
    auto &datas = std::get<std::vector<std::shared_ptr<DataType>>>(this->batchSendQueues_);
    for (auto data : datas) {
      StateSender<DataType>::readyList()->push(data);
    }
    datas.clear();
  }

  // @brief Call `flushResults` for all the types.
  void flushResults() { (flushResults<Outputs>(), ...); }
};
}
}

#endif //HEDGEHOG_STATE_MULTI_SENDERS_H_
