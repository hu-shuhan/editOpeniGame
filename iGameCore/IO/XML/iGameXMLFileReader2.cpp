//
// Created by m_ky on 2024/11/10.
//

/**
 * @class   iGameXMLFileReader2
 * @brief   iGameXMLFileReader2's brief
 */

#include "iGameXMLFileReader2.h"

IGAME_NAMESPACE_BEGIN
bool iGame::XMLFileReader2::InitXMLStructure() {
//    m_XmlHelper = XMLHelper::New();

    return false;
}

bool iGame::XMLFileReader2::Execute() {
    InitXMLStructure();
    return true;
}

bool iGame::XMLFileReader2::Parsing() {
    return false;
}

bool XMLFileReader2::ReadRootNode() {
    return false;
}
IGAME_NAMESPACE_END