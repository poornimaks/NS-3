/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "drop-tail-queue.h"
#include "ns3/simulator.h"
NS_LOG_COMPONENT_DEFINE ("DropTailQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DropTailQueue)
  ;

TypeId DropTailQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::DropTailQueue")
    .SetParent<Queue> ()
    .AddConstructor<DropTailQueue> ()
    .AddAttribute ("Mode", 
                   "Whether to use bytes (see MaxBytes) or packets (see MaxPackets) as the maximum queue size metric.",
                   EnumValue (QUEUE_MODE_PACKETS),
                   MakeEnumAccessor (&DropTailQueue::SetMode),
                   MakeEnumChecker (QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets accepted by this DropTailQueue.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&DropTailQueue::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this DropTailQueue.",
                   UintegerValue (100 * 65535),
                   MakeUintegerAccessor (&DropTailQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
  ;

  return tid;
}

DropTailQueue::DropTailQueue () :
  Queue (),
  m_packets (),
  m_bytesInQueue (0)
{
  NS_LOG_FUNCTION (this);
}

DropTailQueue::~DropTailQueue ()
{
  NS_LOG_FUNCTION (this);
}

void
DropTailQueue::SetMode (DropTailQueue::QueueMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

DropTailQueue::QueueMode
DropTailQueue::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

bool 
DropTailQueue::DoEnqueue (Ptr<Packet> p)
{
	struct queue temp;
	
  NS_LOG_FUNCTION (this << p);

  if (m_mode == QUEUE_MODE_PACKETS && (m_packets.size () >= m_maxPackets))
    {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      Drop (p);
      return false;
    }

  if (m_mode == QUEUE_MODE_BYTES && (m_bytesInQueue + p->GetSize () >= m_maxBytes))
    {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      Drop (p);
      return false;
    }

  m_bytesInQueue += p->GetSize ();
  
  //getting the previous exp leave time
  
  temp.p=p;
  temp.arr_time=Simulator::Now().GetSeconds();
  //need to get bit rate!!
  temp.exp_leave_time=temp.arr_time+p->GetSize()/1;
  //m_packets.push_back (temp);

//now insert it into the apt position

std::deque<struct queue>::iterator it=m_packets.begin();
NS_LOG_UNCOND("size of arrived packet:"<<p->GetSize()<<" queue size: "<<m_packets.size()<<" arrival time"<<Simulator::Now().GetSeconds());

if(!m_packets.empty())
{
	
	struct queue temp2;int pos=0;
	for (it=m_packets.begin(); it!=m_packets.end(); ++it,pos++)
	{
		temp2=*it;
	//	NS_LOG_UNCOND(temp2.exp_leave_time);NS_LOG_UNCOND(temp2.prev_leave_time);NS_LOG_UNCOND(temp.exp_leave_time);
		if(temp2.exp_leave_time>temp.exp_leave_time&&temp.exp_leave_time>temp2.prev_leave_time)
		{
			
			break;
		}
		
	}
	//NS_LOG_UNCOND("insertd after this one");
//	if(pos==1)
	//{
		//temp2=*it;
	//	NS_LOG_UNCOND("+1");
	    temp.exp_leave_time=temp2.prev_leave_time+temp.p->GetSize();
		temp.prev_leave_time=temp2.exp_leave_time;
		//NS_LOG_UNCOND("update time of this prev leave one as"<<temp2.exp_leave_time);
		it=m_packets.insert(it,temp);
		//update
		if(it!=m_packets.end()) // if u have another one next - check for single pack q
		{
			it++;
			for(;it!=m_packets.end();it++,pos++)
			{
				temp2=*it;
				temp2.prev_leave_time=m_packets.at(pos).exp_leave_time;
			}
		}
		
//	}
	/*else
	{
		
		temp2=*it;//NS_LOG_UNCOND(temp2.p->GetSize());
		temp.prev_leave_time=temp2.exp_leave_time;
		//NS_LOG_UNCOND("update time of this prev leave one as"<<temp2.exp_leave_time);
		it=m_packets.insert(it-1,temp);
		it++;
		for(;it!=m_packets.end();it++,pos++)
		{
			temp2=*it;
			temp2.prev_leave_time=m_packets.at(pos).exp_leave_time;
		}
	}
   */
}
else
{
//NS_LOG_UNCOND("first push");	
	temp.prev_leave_time=0;
	m_packets.push_back(temp);
}

/*
if(m_packets.size()==0)
{
	m_packets.insert(it,temp);
}
else 
{
	m_packets.insert(it-1,temp);
}
*/
/*
if(m_packets.size()!=0)
  {
	  temp.prev_leave_time=m_packets.back().exp_leave_time;
  }
  else
  {
	  
  }
  
*/
  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

Ptr<Packet>
DropTailQueue::DoDequeue (void)
{
	struct queue temp;
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }
    
    
   temp= m_packets.front ();
  Ptr<Packet> p =temp.p;
  m_packets.pop_front ();
  //update prev_leave_time
  if(!m_packets.empty())
  {
	  m_packets.front().prev_leave_time=Simulator::Now().GetSeconds();
	 // NS_LOG_UNCOND(" updated leave time of prev"<<m_packets.front().prev_leave_time);
  }
  m_bytesInQueue -= p->GetSize ();
  
  NS_LOG_LOGIC ("Popped " << p);
	
  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

Ptr<const Packet>
DropTailQueue::DoPeek (void) const
{
	struct queue temp;
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

 temp = m_packets.front ();
Ptr<Packet> p =temp.p;
  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

} // namespace ns3

