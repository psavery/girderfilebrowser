//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
// .NAME utils.h
// .SECTION Description
// .SECTION See Also

#ifndef girderfilebrowser_utils_h
#define girderfilebrowser_utils_h

class QNetworkReply;
class QByteArray;
class QString;

namespace cumulus
{
QString handleGirderError(QNetworkReply* reply, const QByteArray& bytes);
}

#endif
