/*
 * Copyright (c) 2014 Natale Patriciello, <natale.patriciello@gmail.com>
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
 *
 */

#include "tcp-adaptive-reno.h"

#include "tcp-socket-state.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpAdaptiveReno");
NS_OBJECT_ENSURE_REGISTERED(TcpAdaptiveReno);

TypeId
TcpAdaptiveReno::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpAdaptiveReno")
                            .SetParent<TcpWestwoodPlus>()
                            .AddConstructor<TcpAdaptiveReno>()
                            .SetGroupName("Internet");
    return tid;
}

TcpAdaptiveReno::TcpAdaptiveReno()
    : TcpWestwoodPlus(),
      m_ackCnt(0)
{
    NS_LOG_FUNCTION(this);
}

TcpAdaptiveReno::TcpAdaptiveReno(const TcpAdaptiveReno& sock)
    : TcpWestwoodPlus(sock),
      m_ackCnt(sock.m_ackCnt),
      m_baseWnd(0),
      m_probeWnd(0),
      m_smoothingFactor(0)
{
    NS_LOG_FUNCTION(this);
}

TcpAdaptiveReno::~TcpAdaptiveReno()
{
    NS_LOG_FUNCTION(this);
}

Ptr<TcpCongestionOps>
TcpAdaptiveReno::Fork()
{
    return CopyObject<TcpAdaptiveReno>(this);
}

std::string
TcpAdaptiveReno::GetName() const
{
    return "TcpAdaptiveReno";
}

void
TcpAdaptiveReno::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << packetsAcked << rtt);

    if (rtt.IsZero())
    {
        NS_LOG_WARN("RTT measured is zero!");
        return;
    }

    m_minRtt = std::min(m_minRtt, rtt);
    m_ackedSegments += packetsAcked;
    m_lastRtt = rtt;

    EstimateBW(rtt, tcb);
}

// On packet drop
uint32_t
TcpAdaptiveReno::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this << tcb << bytesInFlight);

    { // Calculate RTT_cong
        double a = m_smoothingFactor;
        m_congRtt = a * m_congRtt + (1 - a) * m_lastRtt;
        m_smoothingFactor = 0.85; // Initially a was 0. Update it.
    }

    double c = EstimateCongestionLevel(m_lastRtt);
    m_baseWnd = std::max(2 * tcb->m_segmentSize, uint32_t(tcb->m_cWnd / (1 + c)));
    m_probeWnd = 0;

    NS_LOG_LOGIC("CurrentBW: " << m_currentBW << " cwnd: " << tcb->m_cWnd << " new: " << 1 + c
                               << " " << tcb->m_cWnd.Get());

    return m_baseWnd;
}

/**
 * \brief Congestion avoidance of TcpAdaptiveReno
 *
 * \param tcb internal congestion tcb
 * \param segmentsAcked count of segments acked
 */
void
TcpAdaptiveReno::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (segmentsAcked > 0)
    {
        // Maximum segment size is assumed to be the square of original segment size
        double MSS = static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize);
        double adder = MSS / tcb->m_cWnd.Get();
        adder = std::max(1.0, adder);
        m_baseWnd += static_cast<uint32_t>(adder);

        double m_incWnd = EstimateIncWnd(tcb);
        m_probeWnd = std::max(0.0, m_probeWnd + m_incWnd / tcb->m_cWnd.Get());
        tcb->m_cWnd = m_baseWnd + m_probeWnd;
        NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                     << tcb->m_ssThresh << " w_base " << m_baseWnd
                                                     << " w_probe " << m_probeWnd);
    }
}

double
TcpAdaptiveReno::EstimateIncWnd(Ptr<TcpSocketState> tcb)
{
    if (m_congRtt == Time::Max()) // No congestion occurs yet
    {
        return 0;
    }
    uint32_t M = 1000;
    double MSS = static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize);
    double m_incWnd_max = (static_cast<uint32_t>(m_currentBW.Get().GetBitRate()) / (8.0 * M)) * MSS;
    double alpha = 10;
    double beta = 2 * m_incWnd_max * (1 / alpha - (1 / alpha + 1) / std::exp(alpha));
    double gamma = 1 - 2 * m_incWnd_max * (1 / alpha - (1 / alpha + 0.5) / std::exp(alpha));
    double c = EstimateCongestionLevel(m_lastRtt);
    return m_incWnd_max / exp(c * alpha) + c * beta + gamma;
}

double
TcpAdaptiveReno::EstimateCongestionLevel(const Time& rtt)
{
    return std::min(1.0,
                    static_cast<double>(rtt.GetPicoSeconds() - m_minRtt.GetPicoSeconds()) /
                        (m_congRtt.GetPicoSeconds() - m_minRtt.GetPicoSeconds()));
}

} // namespace ns3
