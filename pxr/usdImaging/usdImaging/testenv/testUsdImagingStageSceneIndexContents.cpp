//
// Copyright 2024 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/imaging/hd/dataSource.h"

#include <iostream>
#include <fstream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

class PrimListener : public HdSceneIndexObserver
{
public:
    void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override {
        for (const AddedPrimEntry &entry : entries) {
            _prims.insert(entry.primPath);
        }
    }

    void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override {
        for (const RemovedPrimEntry &entry : entries) {
            _prims.erase(entry.primPath);
        }
    }

    void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override {
    }

    void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override {
        ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
    }

    SdfPathSet const& GetPrimPaths() { return _prims; }

private:
    SdfPathSet _prims;
};

static bool
_IsAddressChar(const char ch)
{
    return (ch >= '0' && ch <= '9') 
        || (ch >= 'a' && ch <= 'f')
        || (ch >= 'A' && ch <= 'F')
        || (ch == 'x');
}

static
std::string
_CleanOutputForDiff(std::string inputStr)
{
    // state machine to detect 0x[0-9a-fA-F]* or "@ [0-9a-fA-F]"
    enum _Mode
    {
        DEFAULT,
        FOUND_0,
        FOUND_AT,
        PTR,
    };

    std::string outputStr;
    size_t numPtrDigits = 0;

    _Mode mode = DEFAULT;
    for (const char& ch: inputStr) {
        switch (mode) {
        case DEFAULT:
            if (ch == '0') {
                mode = FOUND_0;
            } else if (ch == '@') {
                mode = FOUND_AT;
            }
            outputStr += ch;
            break;
        case FOUND_0:
            if (ch == 'x') {
                mode = PTR;
            } else {
                mode = DEFAULT;
            }
            outputStr += ch;
            break;
        case FOUND_AT:
            if (ch == ' ') {
                mode = PTR;
            } else {
                mode = DEFAULT;
            }
            outputStr += ch;
            break;
        case PTR:
            if (_IsAddressChar(ch)) {
                outputStr += 'X';
                numPtrDigits++;
            } else {
                // After processing pointer, add digits until hitting 16 for 
                // size consistency.
                outputStr.append(16 - numPtrDigits, 'X');
                outputStr += ch;
                numPtrDigits = 0;
                mode = DEFAULT;
            }
            break;
        }
    }
    return outputStr;
}

int main(int argc, char**argv)
{
    if (argc != 3) {
        std::cout << "Usage: testStageSceneIndexContents <file.usd> <out.txt>\n";
        return -1;
    }

    UsdStageRefPtr stage = UsdStage::Open(argv[1]);
    if (!TF_VERIFY(stage)) {
        return -1;
    }

    UsdImagingStageSceneIndexRefPtr inputSceneIndex =
        UsdImagingStageSceneIndex::New();
    if (!TF_VERIFY(inputSceneIndex)) {
        return -1;
    }

    HdSceneIndexBase &terminalScene = *inputSceneIndex;
    PrimListener primListener;
    terminalScene.AddObserver(HdSceneIndexObserverPtr(&primListener));

    inputSceneIndex->SetStage(stage);


    // XXX: time from args?
    inputSceneIndex->SetTime(UsdTimeCode::EarliestTime());

    std::ostringstream ss;
    for (SdfPath const& primPath : primListener.GetPrimPaths()) {
        HdSceneIndexPrim prim = terminalScene.GetPrim(primPath);
        ss << "<" << primPath << "> type = " << prim.primType << "\n";
        HdDebugPrintDataSource(ss, prim.dataSource, 1);
    }

    std::ofstream output(argv[2]);
    if (!TF_VERIFY(output.is_open())) {
        return -1;
    }
    output << _CleanOutputForDiff(ss.str());
    output.close();
    return 0;
}
