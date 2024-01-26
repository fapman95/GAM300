#include "AudioComponent.hxx"
#include "../TypeConversion.hxx"

namespace ScriptAPI
{
	AudioComponent::AudioComponent(TDS::EntityID ID) : entityID(ID), transform(TransformComponent(ID))
	{

	}

	TDS::EntityID AudioComponent::GetEntityID()
	{
		return entityID;
	}

	void AudioComponent::set3DCoords(float x, float y, float z)
	{
		pos = Vector3(x, y, z);
	}

	void AudioComponent::set3DCoords(Vector3 in_pos)
	{
		pos = in_pos;
	}

	bool AudioComponent::isLoaded()
	{
		return (whatState == TDS::SOUND_LOADED);
	}

	bool AudioComponent::is3D()
	{
		return isit3D;
	}

	bool AudioComponent::isLoop()
	{
		return isitLoop;
	}

	bool AudioComponent::isMuted()
	{
		return isitMuted;
	}

	bool AudioComponent::isPlaying()
	{
		return (whatState == TDS::SOUND_PLAYING);
	}

	bool AudioComponent::isPaused()
	{
		return (whatState == TDS::SOUND_PAUSE);
	}

	bool AudioComponent::finished(System::String^ str_path)
	{
		return TDS::proxy_audio_system::checkifdone(toStdString(str_path));
	}

	Vector3 AudioComponent::get3DCoords()
	{
		return pos;
	}

	snd AudioComponent::getState()
	{
		return whatState;
	}

	unsigned int AudioComponent::getUniqueID()
	{
		return uniqueID;
	}

	unsigned int AudioComponent::getMSLength()
	{
		return MSLength;
	}

	std::string AudioComponent::getFilePath()
	{
		return filePath;
	}

	const char* AudioComponent::getFilePath_inChar()
	{
		return filePath.c_str();
	}

	void AudioComponent::setFilePath(System::String^ str_path)
	{
		filePath = toStdString(str_path);
	}

	float AudioComponent::getX()
	{
		return pos.X;
	}

	float AudioComponent::getY()
	{
		return pos.Y;
	}

	float AudioComponent::getZ()
	{
		return pos.Z;
	}

	float AudioComponent::getReverbAmount()
	{
		return ReverbAmount;
	}

	float AudioComponent::getVolume()
	{
		return volume;
	}

	void AudioComponent::setVolume(float vol)
	{
		volume = vol;
	}

	void AudioComponent::setMSLength(unsigned int len)
	{
		MSLength = len;
	}

	void AudioComponent::setState(snd setting)
	{
		whatState = setting;
	}

	void AudioComponent::setLoop(bool condition)
	{
		isitLoop = condition;
	}

	void AudioComponent::set3D(bool condition)
	{
		isit3D = condition;
	}

	void AudioComponent::setMute(bool condition)
	{
		isitMuted = condition;
	}

	void AudioComponent::play(System::String^ pathing)
	{
		TDS::proxy_audio_system::ScriptPlay(toStdString(pathing));
	}

	void AudioComponent::playQueue()
	{
		TDS::proxy_audio_system::Play_queue();
	}

	void AudioComponent::pause(System::String^ pathing)
	{
		TDS::proxy_audio_system::ScriptPause(toStdString(pathing));
	}

	void AudioComponent::stop(System::String^ pathing)
	{
		TDS::proxy_audio_system::ScriptStop(toStdString(pathing));
	}

	void AudioComponent::Queue(System::String^ str)
	{
		TDS::proxy_audio_system::Add_to_Queue(toStdString(str));
	}

	void AudioComponent::clearQueue()
	{
		TDS::proxy_audio_system::Clear_queue();
	}

	void AudioComponent::SetEntityID(TDS::EntityID id)
	{
		entityID = id;
	}

	void AudioComponent::SetEnabled(bool enabled)
	{
		TDS::ecs.setEntityIsEnabled(GetEntityID(), enabled);
	}

	//unique ID
	unsigned int AudioComponent::uniqueID::get()
	{
		return TDS::GetSoundInfo(entityID)->getUniqueID();
	}

	//MS Length
	unsigned int AudioComponent::MSLength::get()
	{
		return TDS::GetSoundInfo(entityID)->getMSLength();
	}
	void AudioComponent::MSLength::set(unsigned int value)
	{
		TDS::GetSoundInfo(entityID)->setMSLength(value);
	}

	//file path
	std::string AudioComponent::filePath::get()
	{
		return TDS::GetSoundInfo(entityID)->getFilePath();
	}
	void AudioComponent::filePath::set(std::string value)
	{
		TDS::GetSoundInfo(entityID)->setFilePath(value);
	}

	//loop
	bool AudioComponent::isitLoop::get()
	{
		return TDS::GetSoundInfo(entityID)->isLoop();
	}
	void AudioComponent::isitLoop::set(bool value)
	{
		TDS::GetSoundInfo(entityID)->setLoop(value);
	}

	//3D boolean
	bool AudioComponent::isit3D::get()
	{
		return TDS::GetSoundInfo(entityID)->is3D();
	}
	void AudioComponent::isit3D::set(bool value)
	{
		TDS::GetSoundInfo(entityID)->set3D(value);
	}

	//muted
	bool AudioComponent::isitMuted::get()
	{
		return TDS::GetSoundInfo(entityID)->isMuted();
	}
	void AudioComponent::isitMuted::set(bool value)
	{
		TDS::GetSoundInfo(entityID)->setMute(value);
	}

	//state of sound info
	snd AudioComponent::whatState::get()
	{
		return TDS::GetSoundInfo(entityID)->getState();
	}
	void AudioComponent::whatState::set(snd value)
	{
		TDS::GetSoundInfo(entityID)->setState(value);
	}

	//3D position
	Vector3 AudioComponent::pos::get()
	{
		return Vector3(TDS::GetSoundInfo(entityID)->get3DCoords());
	}
	void AudioComponent::pos::set(Vector3 value)
	{
		TDS::GetSoundInfo(entityID)->set3DCoords(value.X, value.Y, value.Z);
	}

	//volume
	float AudioComponent::volume::get()
	{
		return TDS::GetSoundInfo(entityID)->getVolume();
	}
	void AudioComponent::volume::set(float value)
	{
		TDS::GetSoundInfo(entityID)->setVolume(value);
	}

	//Reverb
	float AudioComponent::ReverbAmount::get()
	{
		return TDS::GetSoundInfo(entityID)->getReverbAmount();
	}
	void AudioComponent::ReverbAmount::set(float value)
	{
		TDS::GetSoundInfo(entityID)->setReverbAmount(value);
	}
}