# Welcome to Tobii Stream Engine samples and documentation

  This repository contains samples and [documentation](https://tobii.github.io/stream_engine) to show developers how to
  use our API to build interactive _eye tracking_ enabled games and applications.

  These samples are minimal samples designed to showcase the basic use cases of stream engine.

  Note that Tobii offers several SDK packages targeted at different programming languages and frameworks, so be sure to
  pick the one that fits your needs best. Stream engine is the lowest eye tracking api supplied by Tobii and it is
  mainly intended for advanced users. More developer related information can be found on [Tobii Developer Zone](http://developer.tobii.com/).

# Contact

  If you have problems, questions, ideas, or suggestions, please use the forums on the Tobii Developer Zone (link below).
  That's what they are for!

  Visit the Tobii Developer Zone web site for the latest news and downloads:

  http://developer.tobii.com/

# Compatibility

  Currently Tobii Stream Engine only supports Windows.

# Dependencies
  Binaries are available on NuGet:
  
  * [Stream Engine C# NuGet package](https://www.nuget.org/packages/Tobii.StreamEngine/)
  
  * [Stream Engine C/C++ Native NuGet package](https://www.nuget.org/packages/Tobii.StreamEngine.Native/)

  or as zip archive from the Developer Zone:
  * [Stream Engine C/C++ binary archive](http://developer.tobii.com/downloads/)

# License

  The sample source code in this project is licensed under the MIT License - please see the LICENSE file for details.
  Samples requires Tobii proprietary binaries to access the eye tracker (available on www.nuget.org and [Tobii Developer Zone](http://developer.tobii.com/)).
  Note that these referenced libraries are covered by a [separate license agreement](https://developer.tobii.com/license-agreement/).

# Revision history

Stream Engine change log below:

What's new in version 2.2.1:

	* Added new fault and warning notifications, which is triggered if the fault or warning state changes.
	* Added ability to query fault and warning state through new function tobii_get_state_string() which will return a comma separated string.
	* New stream subscription return code TOBII_ERROR_TOO_MANY_SUBSCRIBERS, which is returned by the subscribe function if the tracker has reached its maximum allowed subscribers.
	* Add combined gaze data to wearable stream and capability TOBII_CAPABILITY_COMBINED_GAZE_VR.
	* Enable services interface on Linux, which enables headpose and tobii_engine Api if a Tobii Engine for Linux is installed.


What's new in version 2.0.2:

	* tobii_device_create no longer supports null URL as input parameter, this will result in a TOBII_ERROR_INVALID_PARAMETER return code.
	* Ability to enumerate IS1 devices has been removed.
	* Default enumeration using tobii_enumerate_local_device_urls no longer includes IS2.
	* Disable emulated presence for all trackers that do not support a native presence stream, except for IS3.
	* tobii_process_callbacks has been split into two separate functions:
		* tobii_device_process_callbacks, which has the same arguments and functionality as that of the old function.
		* tobii_engine_process_callbacks, which processes the callbacks from the new tobii_engine_t connection type.
	* tobii_wait_for_callbacks now takes more arguments both a tobii_engine_t instance and multiple tobii_device_t instances.
	* tobii_reconnect has been split into two separate functions:
		* tobii_device_reconnect, which has the same arguments and functionality as that of the old function.
		* tobii_engine_reconnect, which reconnects the new tobii_engine_t connection type.
	* New tobii_engine_t connection type that enables direct communication with the Tobii Service, can be found in new header file tobii_engine.h. Features include:
		* Utility functions, very similar to the tobii_device_t functions; create, destroy, reconnect, process_callbacks and clear_callback_buffers.
		* Ability to query connected devices via the tobii_enumerate_devices function.
		* Subscription to changes in connected devices via the new tobii_device_list_change_subscription.
	* New error return codes have been added to the Api:
		* TOBII_ERROR_CALLBACK_IN_PROGRESS, returned from functions who are called from within a callback context.
		* TOBII_ERROR_CONFLICTING_API_INSTANCES, returned from wait_for_callbacks if engine and device instance was created using different api instances.
		* TOBII_ERROR_CALIBRATION_BUSY, returned from tobii_calibration_start tobii_calibration_apply when another client is already calibrating.
	* New notifications available:
		* TOBII_NOTIFICATION_TYPE_CALIBRATION_ENABLED_EYE_CHANGED, sent when the enabled eye has changed.
		* TOBII_NOTIFICATION_TYPE_CALIBRATION_ID_CHANGED, sent when a new calibration has been set.
	* New states available for query through the tobii_get_state_* functions
		* TOBII_STATE_CALIBRATION_ID, queries the calibration ID/Hash of the active calibration, 0 if no calibration is set.
		* TOBII_STATE_CALIBRATION_ACTIVE, queries if the tracker is in calibration state.
	* New calibration id concept, where the client can query and receive notification on the calibration id, which is a unique identifier of the calibration blob.
	* Fixed behavior of tobii_device_clear_callback_buffers.


What's new in version 1.1.1:

	* Fix for possible command mutex starvation, where a command can be delayed for long periods of time during certain conditions.
	* Fix for possible crashes in service transport communication.


What's new in version 1.1.0:

	* New Stream engine consumer zip package, which includes the public consumer parts.
	* Fix C# binding for tobii_set_lens_configuration, was missing a ref modifier.
	* tobii_subscribe_* now returns invalid parameter when supplying a null callback
	* Fixed the trace functionality


What's new in version 1.0.0:

	* First public release of stream engine
