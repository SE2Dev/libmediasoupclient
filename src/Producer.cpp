#define MSC_CLASS "Producer"

#include "Producer.hpp"
#include "Logger.hpp"
#include "MediaSoupClientErrors.hpp"

using json = nlohmann::json;

namespace mediasoupclient
{
	Producer::Producer(
	  Producer::PrivateListener* privateListener,
	  Producer::Listener* listener,
	  const std::string& id,
	  const std::string& localId,
	  webrtc::RtpSenderInterface* rtpSender,
	  webrtc::MediaStreamTrackInterface* track,
	  const json& rtpParameters,
	  const bool stopTracks,
	  const bool zeroRtpOnPause,
	  const bool disableTrackOnPause,
	  const json& appData)
	  : privateListener(privateListener), listener(listener), id(id), localId(localId),
	    rtpSender(rtpSender), track(track), rtpParameters(rtpParameters), stopTracks(stopTracks),
	    zeroRtpOnPause(zeroRtpOnPause), disableTrackOnPause(disableTrackOnPause), appData(appData)
	{
		MSC_TRACE();
	}

	const std::string& Producer::GetId() const
	{
		MSC_TRACE();

		return this->id;
	}

	const std::string& Producer::GetLocalId() const
	{
		MSC_TRACE();

		return this->localId;
	}

	bool Producer::IsClosed() const
	{
		MSC_TRACE();

		return this->closed;
	}

	std::string Producer::GetKind() const
	{
		MSC_TRACE();

		return this->track->kind();
	}

	webrtc::RtpSenderInterface* Producer::GetRtpSender() const
	{
		MSC_TRACE();

		return this->rtpSender;
	}

	webrtc::MediaStreamTrackInterface* Producer::GetTrack() const
	{
		MSC_TRACE();

		return this->track;
	}

	const json& Producer::GetRtpParameters() const
	{
		MSC_TRACE();

		return this->rtpParameters;
	}

	bool Producer::IsPaused() const
	{
		MSC_TRACE();

		return this->paused;
	}

	uint8_t Producer::GetMaxSpatialLayer() const
	{
		MSC_TRACE();

		return this->maxSpatialLayer;
	}

	json& Producer::GetAppData()
	{
		MSC_TRACE();

		return this->appData;
	}

	/**
	 * Closes the Producer.
	 */
	void Producer::Close()
	{
		MSC_TRACE();

		if (this->closed)
			return;

		this->closed = true;

		this->privateListener->OnClose(this);
	}

	json Producer::GetStats() const
	{
		if (this->closed)
			MSC_THROW_INVALID_STATE_ERROR("Producer closed");

		return this->privateListener->OnGetStats(this);
	}

	/**
	 * Pauses sending media.
	 */
	void Producer::Pause()
	{
		MSC_TRACE();

		if (this->closed)
		{
			MSC_ERROR("Producer closed");

			return;
		}

		this->paused = true;

		if (this->track != nullptr && this->disableTrackOnPause)
		{
			this->track->set_enabled(false);
		}

		if (this->zeroRtpOnPause)
		{
			this->privateListener->OnReplaceTrack(this, nullptr);
		}
	}

	/**
	 * Resumes sending media.
	 */
	void Producer::Resume()
	{
		MSC_TRACE();

		if (this->closed)
		{
			MSC_ERROR("Producer closed");

			return;
		}

		this->paused = false;

		if (this->track != nullptr && this->disableTrackOnPause)
		{
			this->track->set_enabled(true);
		}

		if (this->zeroRtpOnPause)
		{
			this->privateListener->OnReplaceTrack(this, this->track);
		}
	}

	/**
	 * Replaces the current track with a new one.
	 */
	void Producer::ReplaceTrack(webrtc::MediaStreamTrackInterface* track)
	{
		MSC_TRACE();

		if (this->closed)
			MSC_THROW_INVALID_STATE_ERROR("Producer closed");
		else if (track && track->state() == webrtc::MediaStreamTrackInterface::TrackState::kEnded)
			MSC_THROW_INVALID_STATE_ERROR("track ended");

		// Do nothing if this is the same track as the current handled one.
		if (track == this->track)
		{
			MSC_DEBUG("same track, ignored");

			return;
		}

		// May throw.
		this->privateListener->OnReplaceTrack(this, track);

		auto paused = IsPaused();

		// Set the new track.
		this->track = track;

		// If this Producer was paused/resumed and the state of the new
		// track does not match, fix it.
		if (this->track != nullptr)
		{
			if (!paused)
				this->track->set_enabled(true);
			else
				this->track->set_enabled(false);
		}
	}

	/**
	 * Sets the max spatial layer to be sent.
	 */
	void Producer::SetMaxSpatialLayer(const uint8_t spatialLayer)
	{
		MSC_TRACE();

		if (this->closed)
			MSC_THROW_INVALID_STATE_ERROR("Producer closed");
		else if (this->track->kind() != "video")
			MSC_THROW_TYPE_ERROR("not a video Producer");

		if (spatialLayer == this->maxSpatialLayer)
			return;

		// May throw.
		this->privateListener->OnSetMaxSpatialLayer(this, spatialLayer);

		this->maxSpatialLayer = spatialLayer;
	}

	/**
	 * Sets the RTP encoding parameters for this producer.
	 */
	void Producer::SetRtpEncodingParameters(std::vector<webrtc::RtpEncodingParameters> parameters)
	{
		MSC_TRACE();

		if (this->closed)
			MSC_THROW_INVALID_STATE_ERROR("Producer close");
		else if (this->track->kind() != "video")
			MSC_THROW_TYPE_ERROR("not a video Producer");

		this->privateListener->OnSetRtpEncodingParameters(this, parameters);
	}

	/**
	 * Transport was closed.
	 */
	void Producer::TransportClosed()
	{
		MSC_TRACE();

		if (this->closed)
			return;

		this->closed = true;

		this->listener->OnTransportClose(this);
	}
} // namespace mediasoupclient
