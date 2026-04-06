// NIST-developed software is provided by NIST as a public service. You may use, copy and distribute copies of the
// software in any medium, provided that you keep intact this entire notice. You may improve, modify and create
// derivative works of the software or any portion of the software, and you may copy and distribute such modifications
// or works. Modified works should carry a notice stating that you changed the software and should note the date and
// nature of any such change. Please explicitly acknowledge the National Institute of Standards and Technology as the
// source of the software. NIST-developed software is expressly provided "AS IS." NIST MAKES NO WARRANTY OF ANY KIND,
// EXPRESS, IMPLIED, IN FACT OR ARISING BY OPERATION OF LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR
// WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE
// CORRECTED. NIST DOES NOT WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE SOFTWARE OR THE RESULTS
// THEREOF, INCLUDING BUT NOT LIMITED TO THE CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE. You
// are solely responsible for determining the appropriateness of using and distributing the software and you assume
// all risks associated with its use, including but not limited to the risks and costs of program errors, compliance
// with applicable laws, damage to or loss of data, programs or equipment, and the unavailability or interruption of
// operation. This software is not intended to be used in any situation where a failure could cause risk of injury or
// damage to property. The software developed by NIST employees is not subject to copyright protection within the
// United States.


#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "../../hedgehog/hedgehog.h"

// Test data structure for basic test
struct BasicData {
    int value;
    BasicData(int v) : value(v) {}
};

// Test data structure for complex test - multiple types
struct IntData {
    int value;
    IntData(int v) : value(v) {}
};

struct DoubleData {
    double value;
    DoubleData(double v) : value(v) {}
};

struct StringData {
    std::string value;
    StringData(std::string v) : value(std::move(v)) {}
};

// Helper function to create a vector of shared pointers
template<typename T, typename... Args>
std::vector<std::shared_ptr<T>> makeSharedVector(Args&&... args) {
    return {std::make_shared<T>(std::forward<Args>(args))...};
}

void batchAddResultBasicTest() {
    // Basic test: single task with multiple results
    hh::Graph<1, BasicData, BasicData> g;
    auto task = std::make_shared<hh::LambdaTask<1, BasicData, BasicData>>();

    // Lambda that uses batchAddResult to send multiple results
    task->setLambda<BasicData>([](std::shared_ptr<BasicData> data, auto self) {
        // Create multiple results to send in batch
        auto results = makeSharedVector<BasicData>(
            BasicData(data->value * 2),
            BasicData(data->value * 3),
            BasicData(data->value * 4)
        );
        self.batchAddResult(results);
    });

    g.inputs(task);
    g.outputs(task);
    g.executeGraph();

    // Push test data
    g.pushData(std::make_shared<BasicData>(5));
    g.finishPushingData();

    // Collect results
    std::vector<std::shared_ptr<BasicData>> results;
    while (auto res = g.getBlockingResult()) {
        // Extract the BasicData from the tuple (single element)
        results.push_back(std::get<0>(*res));
    }

    g.waitForTermination();

    // Should have 3 results: 5*2=10, 5*3=15, 5*4=20
    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(results[0]->value, 10);
    EXPECT_EQ(results[1]->value, 15);
    EXPECT_EQ(results[2]->value, 20);
}

void batchAddResultComplexTest() {
    // Complex test: multiple task types with multiple receivers
    hh::Graph<3, IntData, DoubleData, StringData, IntData, DoubleData, StringData> g;

    // Task 1: processes IntData and produces IntData and DoubleData
    auto task1 = std::make_shared<hh::LambdaTask<3, IntData, DoubleData, StringData, IntData, DoubleData>>();
    task1->setLambda<IntData>([](std::shared_ptr<IntData> data, auto self) {
        // Batch send multiple results of different types
        auto intResults = makeSharedVector<IntData>(
            IntData(data->value * 2),
            IntData(data->value * 3)
        );
        auto doubleResults = makeSharedVector<DoubleData>(
            DoubleData(static_cast<double>(data->value) * 1.5),
            DoubleData(static_cast<double>(data->value) * 2.5)
        );
        self.batchAddResult(intResults);    // First output type (IntData)
        self.batchAddResult(doubleResults); // Second output type (DoubleData)
    });
    // Set dummy lambdas for other input types to avoid bad_function_call
    task1->setLambda<DoubleData>([](std::shared_ptr<DoubleData>, auto) {});
    task1->setLambda<StringData>([](std::shared_ptr<StringData>, auto) {});

    // Task 2: processes DoubleData and produces StringData
    auto task2 = std::make_shared<hh::LambdaTask<3, IntData, DoubleData, StringData, IntData, DoubleData, StringData>>();
    task2->setLambda<DoubleData>([](std::shared_ptr<DoubleData> data, auto self) {
        auto stringResults = makeSharedVector<StringData>(
            StringData("double_" + std::to_string(static_cast<int>(data->value * 10))),
            StringData("value_" + std::to_string(static_cast<int>(data->value)))
        );
        self.batchAddResult(stringResults); // Third output type (StringData)
    });
    // Set dummy lambdas for other input types to avoid bad_function_call
    task2->setLambda<IntData>([](std::shared_ptr<IntData>, auto) {});
    task2->setLambda<StringData>([](std::shared_ptr<StringData>, auto) {});

    // Task 3: processes StringData and produces IntData
    auto task3 = std::make_shared<hh::LambdaTask<3, IntData, DoubleData, StringData, IntData, DoubleData, StringData>>();
    task3->setLambda<StringData>([](std::shared_ptr<StringData> data, auto self) {
        auto intResults = makeSharedVector<IntData>(
            IntData(static_cast<int>(data->value.length())),
            IntData(static_cast<int>(data->value.length() * 2))
        );
        self.batchAddResult(intResults); // First output type (IntData)
    });
    // Set dummy lambdas for other input types to avoid bad_function_call
    task3->setLambda<IntData>([](std::shared_ptr<IntData>, auto) {});
    task3->setLambda<DoubleData>([](std::shared_ptr<DoubleData>, auto) {});

    // Set up connections
    g.inputs(task1);
    g.edges(task1, task2); // Connect task1's DoubleData output to task2's DoubleData input
    g.edges(task2, task3); // Connect task2's StringData output to task3's StringData input
    // Set outputs for all tasks
    g.outputs(task1);
    g.outputs(task2);
    g.outputs(task3);

    g.executeGraph();

    // Push initial data
    g.pushData(std::make_shared<IntData>(5));
    g.finishPushingData();

    // Collect all results
    std::vector<std::shared_ptr<IntData>> intResults;
    std::vector<std::shared_ptr<DoubleData>> doubleResults;
    std::vector<std::shared_ptr<StringData>> stringResults;

    while (auto res = g.getBlockingResult()) {
        if (auto intPtr = std::get_if<std::shared_ptr<IntData>>(&(*res))) {
            intResults.push_back(*intPtr);
        } else if (auto doublePtr = std::get_if<std::shared_ptr<DoubleData>>(&(*res))) {
            doubleResults.push_back(*doublePtr);
        } else if (auto stringPtr = std::get_if<std::shared_ptr<StringData>>(&(*res))) {
            stringResults.push_back(*stringPtr);
        }
    }

    g.waitForTermination();

    // Verify results
    // From initial IntData(5):
    // Task1 produces: IntData(10), IntData(15) and DoubleData(7.5), DoubleData(12.5)
    // Task2 processes DoubleData:
    //   DoubleData(7.5) -> StringData("double_75"), StringData("value_7")
    //   DoubleData(12.5) -> StringData("double_125"), StringData("value_12")
    // Task3 processes StringData:
    //   StringData("double_75") (length 9) -> IntData(9), IntData(18)
    //   StringData("value_7") (length 6) -> IntData(6), IntData(12)
    //   StringData("double_125") (length 11) -> IntData(11), IntData(22)
    //   StringData("value_12") (length 8) -> IntData(8), IntData(16)

    // We should get 2 int results from task1, 4 string results from task2, and 8 int results from task3
    EXPECT_EQ(intResults.size(), 2 + 8); // 2 from task1, 8 from task3
    EXPECT_EQ(doubleResults.size(), 2);  // 2 from task1
    EXPECT_EQ(stringResults.size(), 4);  // 4 from task2

    // Check specific values from task1
    bool foundInt10 = false, foundInt15 = false;
    bool foundDouble7_5 = false, foundDouble12_5 = false;

    for (const auto& res : intResults) {
        if (res->value == 10) foundInt10 = true;
        if (res->value == 15) foundInt15 = true;
    }

    for (const auto& res : doubleResults) {
        if (res->value == 7.5) foundDouble7_5 = true;
        if (res->value == 12.5) foundDouble12_5 = true;
    }

    EXPECT_TRUE(foundInt10);
    EXPECT_TRUE(foundInt15);
    EXPECT_TRUE(foundDouble7_5);
    EXPECT_TRUE(foundDouble12_5);
}

TEST(BatchAddResult, basicTest) {
    ASSERT_NO_THROW(batchAddResultBasicTest());
}

TEST(BatchAddResult, complexTest) {
    ASSERT_NO_THROW(batchAddResultComplexTest());
}
