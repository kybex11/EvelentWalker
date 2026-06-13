// Exports a parsed RBF tree to an XML string (mirrors XmlRbf.cs output shape).
#pragma once

#include <memory>
#include <string>

#include "evw/gamefiles/rbf.h"

namespace evw::gamefiles
{
    // Serializes an RBF node tree to indented XML.
    std::string rbfToXml(const std::shared_ptr<RbfNode>& root);
}
