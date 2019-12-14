/*
  Copyright 2019 www.dev5.cn, Inc. dev5@qq.com
 
  This file is part of X-MSG-IM.
 
  X-MSG-IM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  X-MSG-IM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU Affero General Public License
  along with X-MSG-IM.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <libx-msg-im-org-msg.h>
#include <libx-msg-im-org-db.h>
#include "XmsgImOrg.h"

XmsgImOrg* XmsgImOrg::inst = new XmsgImOrg();

XmsgImOrg::XmsgImOrg()
{

}

XmsgImOrg* XmsgImOrg::instance()
{
	return XmsgImOrg::inst;
}

bool XmsgImOrg::start(const char* path)
{
	Log::setInfo();
	shared_ptr<XmsgImOrgCfg> cfg = XmsgImOrgCfg::load(path); 
	if (cfg == nullptr)
		return false;
	Log::setLevel(cfg->cfgPb->log().level().c_str());
	Log::setOutput(cfg->cfgPb->log().output());
	Xsc::init();
	if (!XmsgImOrgDb::instance()->load()) 
		return false;
	shared_ptr<XscTcpServer> tcpServer(new XscTcpServer(cfg->cfgPb->cgt(), shared_ptr<XmsgImOrgTcpLog>(new XmsgImOrgTcpLog())));
	if (!tcpServer->startup(XmsgImOrgCfg::instance()->xscServerCfg())) 
		return false;
	shared_ptr<XmsgImN2HMsgMgr> priMsgMgr(new XmsgImN2HMsgMgr(tcpServer));
	XmsgImOrgMsg::init(priMsgMgr);
	if (!tcpServer->publish()) 
		return false;
	if (!this->connect2ne(tcpServer))
		return false;
	Xsc::hold([](ullong now)
	{

	});
	return true;
}

bool XmsgImOrg::connect2ne(shared_ptr<XscTcpServer> tcpServer)
{
	for (int i = 0; i < XmsgImOrgCfg::instance()->cfgPb->h2n_size(); ++i)
	{
		auto& ne = XmsgImOrgCfg::instance()->cfgPb->h2n(i);
		if (ne.neg() == X_MSG_AP)
		{
			shared_ptr<XmsgAp> ap(new XmsgAp(tcpServer, ne.addr(), ne.pwd(), ne.alg()));
			ap->connect();
			continue;
		}
		if (ne.neg() == X_MSG_IM_HLR)
		{
			shared_ptr<XmsgImHlr> hlr(new XmsgImHlr(tcpServer, ne.addr(), ne.pwd(), ne.alg()));
			hlr->connect();
			continue;
		}
		LOG_ERROR("unsupported network element group: %s", ne.ShortDebugString().c_str())
		return false;
	}
	return true;
}

XmsgImOrg::~XmsgImOrg()
{

}

