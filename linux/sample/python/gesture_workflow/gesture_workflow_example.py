#!/usr/bin/env python3
"""
Complete Gesture Workflow Example

This file demonstrates the complete workflow for gesture recording and H5 generation:
1. Record gestures from device or use predefined data
2. Save gestures as JSON files
3. Convert JSON files to HDF5 format for playback

Features:
- Record gestures from Sharpa device
- Use predefined gesture data
- Read gestures from external files
- Convert JSON to H5 with flexible options
- Complete end-to-end workflow examples
"""

import sys
import os
import json
import glob
from pathlib import Path
from typing import List, Optional

# Add SDK path (from sample/python/gesture_workflow to src/hardware/hand_sdk/src/python)
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../python'))

# Import gesture recording functions
from sample_record_gesture import (
    GESTURE_DATA,
    record_gesture,
    save_gestures_from_array,
    parse_txt_file_to_gestures,
    HandSide
)

# Import device manager - try multiple import paths
SharpaDeviceManager = None
try:
    # Try from sharpa.sharpa (source code)
    from sharpa.sharpa import SharpaDeviceManager
except (ImportError, AttributeError):
    try:
        # Try from sharpa (installed package)
        from sharpa import SharpaDeviceManager
    except (ImportError, AttributeError):
        try:
            # Fallback to SharpaWaveManager
            from sharpa.sharpa import SharpaWaveManager as SharpaDeviceManager
        except (ImportError, AttributeError):
            try:
                from sharpa import SharpaWaveManager as SharpaDeviceManager
            except ImportError:
                print("Warning: Could not import SharpaDeviceManager or SharpaWaveManager")
                SharpaDeviceManager = None

# Import H5 generation functions
from json_to_h5 import gen_h5

# Import replay functions
# Priority: 1. Local sample version (for standalone SDK), 2. Plugin version (for full SDK)
try:
    # First try local sample version (standalone SDK)
    from h5_replay import H5Replay
    REPLAY_AVAILABLE = True
except ImportError:
    # Fallback to plugin version (full SDK with plugins)
    try:
        import sys
        # Add src path for H5Replay (plugins.replay.h5_replay needs to be imported from src directory)
        # From sample/python/gesture_workflow to src
        src_path = os.path.join(os.path.dirname(__file__), '../../../src')
        if src_path not in sys.path:
            sys.path.insert(0, src_path)
        from plugins.replay.h5_replay import H5Replay
        REPLAY_AVAILABLE = True
    except ImportError:
        # Try direct import as fallback
        try:
            replay_path = os.path.join(os.path.dirname(__file__), '../../../src/plugins/replay')
            if replay_path not in sys.path:
                sys.path.insert(0, replay_path)
            from h5_replay import H5Replay
            REPLAY_AVAILABLE = True
        except ImportError:
            REPLAY_AVAILABLE = False
            # Only print warning if actually trying to use replay
            # print(f"Warning: H5Replay not available. Replay examples will be skipped.")

# ============================================================================
# Helper Function: Wait for Replay Completion
# ============================================================================

def wait_for_replay_completion(replay, step_name="Step"):
    """
    Wait for H5 replay to complete and display progress
    
    Args:
        replay: H5Replay instance
        step_name: Name of the step for logging (default: "Step")
    """
    try:
        import time
        while replay.active or replay.running:
            time.sleep(0.5)
            if replay.active and replay.stamp is not None and len(replay.stamp) > 0:
                with replay.lock:
                    current_idx = replay.replay_index
                # Clamp progress to 0-100%
                if current_idx >= len(replay.stamp):
                    progress = 100.0
                    displayed_idx = len(replay.stamp)
                else:
                    progress = min(100.0, (current_idx / len(replay.stamp)) * 100)
                    displayed_idx = current_idx
                print(f"  Replay progress: {progress:.1f}% ({displayed_idx}/{len(replay.stamp)})", end='\r')
            else:
                # Replay not active or data not ready
                if not replay.active and replay.stamp is not None and len(replay.stamp) > 0:
                    # Replay completed
                    progress = 100.0
                    displayed_idx = len(replay.stamp)
                    print(f"  Replay progress: {progress:.1f}% ({displayed_idx}/{len(replay.stamp)})", end='\r')
                    break
    except KeyboardInterrupt:
        print("\n\n  Replay interrupted by user")
    finally:
        print(f"\n[{step_name}] Stopping replay...")
        replay.close()
        print("✓ Replay stopped")


# ============================================================================
# Example 1: Basic Workflow - Record from Device and Convert to H5
# ============================================================================

def example_record_from_device_to_h5():
    """
    Complete workflow: Record gesture from device -> Save JSON -> Convert to H5
    """
    print("=" * 60)
    print("Example 1: Record from Device and Convert to H5")
    print("=" * 60)
    
    # Step 1: Record gesture from device
    output_dir = "./exports"
    os.makedirs(output_dir, exist_ok=True)
    
    print("\n[Step 1] Recording gesture from device...")
    json_file = record_gesture(
        output_dir=output_dir,
        hand_side=HandSide.RIGHT
    )
    
    if not json_file:
        print("Failed to record gesture from device")
        return
    
    print(f"✓ Gesture saved to: {json_file}")
    
    # Step 2: Convert to H5
    print("\n[Step 2] Converting JSON to H5...")
    gen_h5(
        json_files=[json_file],
        h5_file="./output/gesture_from_device.hdf5",
        interval=1.0,
        interval_unit='s'
    )
    
    print("\n✓ Complete workflow finished!")


# ============================================================================
# Example 2: Use Predefined Data - Generate Multiple Gestures and Convert
# ============================================================================

def example_predefined_data_to_h5():
    """
    Complete workflow: Use predefined GESTURE_DATA -> Save JSON -> Convert to H5
    """
    print("=" * 60)
    print("Example 2: Predefined Data to H5")
    print("=" * 60)
    
    if not GESTURE_DATA:
        print("Error: GESTURE_DATA is empty. Please define gesture data in sample_record_gesture.py")
        return
    
    # Step 1: Save predefined gestures as JSON files
    output_dir = "./exports"
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"\n[Step 1] Saving {len(GESTURE_DATA)} predefined gestures as JSON...")
    saved_files = save_gestures_from_array(
        gesture_array=GESTURE_DATA,
        output_path=output_dir,
        filename_prefix="PredefinedGesture"
    )
    
    if not saved_files:
        print("Failed to save gesture files")
        return
    
    print(f"✓ Saved {len(saved_files)} JSON files")
    
    # Step 2: Convert all JSON files to H5
    print("\n[Step 2] Converting JSON files to H5...")
    gen_h5(
        json_folder=output_dir,
        h5_file="./output/predefined_gestures.hdf5",
        interval=1.0,
        interval_unit='s',
        sort_by='number'
    )
    
    print("\n✓ Complete workflow finished!")


# ============================================================================
# Example 3: Read from External File and Convert
# ============================================================================

def example_external_file_to_h5(txt_file_path: str):
    """
    Complete workflow: Read gestures from external file -> Save JSON -> Convert to H5
    """
    print("=" * 60)
    print("Example 3: External File to H5")
    print("=" * 60)
    
    if not os.path.exists(txt_file_path):
        print(f"Error: File not found: {txt_file_path}")
        return
    
    # Step 1: Parse external file
    print(f"\n[Step 1] Parsing external file: {txt_file_path}")
    gestures = parse_txt_file_to_gestures(txt_file_path)
    
    if not gestures:
        print("Error: No valid gestures found in file")
        return
    
    print(f"✓ Found {len(gestures)} gesture(s)")
    
    # Step 2: Save as JSON files
    output_dir = "./exports"
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"\n[Step 2] Saving gestures as JSON files...")
    saved_files = save_gestures_from_array(
        gesture_array=gestures,
        output_path=output_dir,
        filename_prefix="ExternalGesture"
    )
    
    if not saved_files:
        print("Failed to save gesture files")
        return
    
    print(f"✓ Saved {len(saved_files)} JSON files")
    
    # Step 3: Convert to H5
    print("\n[Step 3] Converting JSON files to H5...")
    gen_h5(
        json_files=saved_files,
        h5_file="./output/external_gestures.hdf5",
        interval=1.5,
        interval_unit='s',
        sort_by='time'
    )
    
    print("\n✓ Complete workflow finished!")


# ============================================================================
# Example 4: Advanced Workflow - Select, Sort, Repeat, and Append
# ============================================================================

def example_advanced_workflow():
    """
    Advanced workflow: Multiple gestures with custom sorting, repetition, and append
    """
    print("=" * 60)
    print("Example 4: Advanced Workflow")
    print("=" * 60)
    
    # Step 1: Generate initial gestures from predefined data
    output_dir = "./exports"
    os.makedirs(output_dir, exist_ok=True)
    
    if not GESTURE_DATA:
        print("Error: GESTURE_DATA is empty")
        return
    
    print(f"\n[Step 1] Saving {len(GESTURE_DATA)} gestures as JSON...")
    saved_files = save_gestures_from_array(
        gesture_array=GESTURE_DATA,
        output_path=output_dir,
        filename_prefix="AdvancedGesture"
    )
    
    if not saved_files:
        print("Failed to save gesture files")
        return
    
    print(f"✓ Saved {len(saved_files)} JSON files")
    
    # Step 2: Create initial H5 file with first 2 gestures, sorted by time
    print("\n[Step 2] Creating initial H5 file with first 2 gestures...")
    initial_files = saved_files[:2] if len(saved_files) >= 2 else saved_files
    
    gen_h5(
        json_files=initial_files,
        h5_file="./output/advanced_workflow.hdf5",
        sort_by='time',
        repeat_count=2,  # Repeat each gesture 2 times
        interval=0.5,
        interval_unit='s',
        samples_per_gesture=8
    )
    
    # Step 3: Append remaining gestures with different settings
    if len(saved_files) > 2:
        print("\n[Step 3] Appending remaining gestures to H5 file...")
        remaining_files = saved_files[2:]
        
        gen_h5(
            json_files=remaining_files,
            h5_file="./output/advanced_workflow.hdf5",
            append=True,  # Append mode
            sort_by='number',
            repeat_count=1,
            interval=1.0,
            interval_unit='s',
            samples_per_gesture=5
        )
    
    print("\n✓ Advanced workflow finished!")


# ============================================================================
# Example 5: Batch Process Multiple Gestures with Custom Settings
# ============================================================================

def example_batch_process():
    """
    Batch process: Record multiple gestures, then convert with custom settings
    """
    print("=" * 60)
    print("Example 5: Batch Process Multiple Gestures")
    print("=" * 60)
    
    output_dir = "./exports"
    os.makedirs(output_dir, exist_ok=True)
    
    # Step 1: Record multiple gestures (simulated - in real use, record from device)
    print("\n[Step 1] Recording multiple gestures...")
    print("(In real scenario, you would record gestures interactively)")
    
    # For demonstration, use predefined data
    if not GESTURE_DATA:
        print("Error: GESTURE_DATA is empty")
        return
    
    saved_files = save_gestures_from_array(
        gesture_array=GESTURE_DATA,
        output_path=output_dir,
        filename_prefix="BatchGesture"
    )
    
    if not saved_files:
        print("Failed to save gesture files")
        return
    
    print(f"✓ Recorded {len(saved_files)} gestures")
    
    # Step 2: Convert with custom settings
    print("\n[Step 2] Converting with custom settings...")
    gen_h5(
        json_files=saved_files,
        h5_file="./output/batch_gestures.hdf5",
        sort_by='number',
        repeat_count=3,  # Repeat each gesture 3 times
        interval=0.8,
        interval_unit='s',
        samples_per_gesture=10  # More samples for smoother playback
    )
    
    print("\n✓ Batch processing finished!")


# ============================================================================
# Example 6: Complete Pipeline with Error Handling
# ============================================================================

def example_complete_pipeline_with_error_handling():
    """
    Complete pipeline with comprehensive error handling
    """
    print("=" * 60)
    print("Example 6: Complete Pipeline with Error Handling")
    print("=" * 60)
    
    try:
        output_dir = "./exports"
        h5_output_dir = "./output"
        
        # Create directories
        os.makedirs(output_dir, exist_ok=True)
        os.makedirs(h5_output_dir, exist_ok=True)
        
        # Step 1: Generate JSON files
        print("\n[Step 1] Generating JSON files from predefined data...")
        if not GESTURE_DATA:
            raise ValueError("GESTURE_DATA is empty")
        
        saved_files = save_gestures_from_array(
            gesture_array=GESTURE_DATA,
            output_path=output_dir,
            filename_prefix="PipelineGesture"
        )
        
        if not saved_files:
            raise RuntimeError("Failed to save gesture files")
        
        print(f"✓ Generated {len(saved_files)} JSON files")
        
        # Step 2: Validate JSON files
        print("\n[Step 2] Validating JSON files...")
        valid_files = []
        for json_file in saved_files:
            try:
                with open(json_file, 'r') as f:
                    data = json.load(f)
                    if 'action' in data and 'state' in data:
                        valid_files.append(json_file)
                    else:
                        print(f"  ⚠ Warning: {json_file} missing required fields")
            except Exception as e:
                print(f"  ✗ Error validating {json_file}: {e}")
        
        if not valid_files:
            raise RuntimeError("No valid JSON files found")
        
        print(f"✓ Validated {len(valid_files)} JSON files")
        
        # Step 3: Convert to H5
        print("\n[Step 3] Converting to H5 format...")
        h5_file = os.path.join(h5_output_dir, "pipeline_gestures.hdf5")
        
        gen_h5(
            json_files=valid_files,
            h5_file=h5_file,
            sort_by='number',
            interval=1.0,
            interval_unit='s'
        )
        
        # Step 4: Verify H5 file
        print("\n[Step 4] Verifying H5 file...")
        import h5py
        with h5py.File(h5_file, 'r') as f:
            if 'stamp' in f:
                num_stamps = len(f['stamp'])
                print(f"  ✓ H5 file contains {num_stamps} time stamps")
            if 'action/position' in f:
                num_actions = len(f['action/position'])
                print(f"  ✓ H5 file contains {num_actions} action records")
            if 'state/position' in f:
                num_states = len(f['state/position'])
                print(f"  ✓ H5 file contains {num_states} state records")
        
        print(f"\n✓ Complete pipeline finished successfully!")
        print(f"  Output H5 file: {h5_file}")
        
    except Exception as e:
        print(f"\n✗ Pipeline failed with error: {e}")
        import traceback
        traceback.print_exc()


# ============================================================================
# Example 7: Interactive Workflow
# ============================================================================

def example_interactive_workflow():
    """
    Interactive workflow: Record gestures one by one, then convert
    """
    print("=" * 60)
    print("Example 7: Interactive Workflow")
    print("=" * 60)
    
    output_dir = "./exports"
    os.makedirs(output_dir, exist_ok=True)
    
    print("\nThis example demonstrates an interactive workflow.")
    print("In a real scenario, you would:")
    print("1. Record gesture 1 from device")
    print("2. Record gesture 2 from device")
    print("3. ...")
    print("4. Convert all recorded gestures to H5")
    
    # Simulate recording multiple gestures
    print("\n[Simulating] Recording gestures...")
    
    if not GESTURE_DATA:
        print("Error: GESTURE_DATA is empty")
        return
    
    # Save gestures one by one
    saved_files = []
    for i, gesture in enumerate(GESTURE_DATA[:3]):  # Use first 3 for demo
        print(f"\n  Recording gesture {i+1}...")
        files = save_gestures_from_array(
            gesture_array=[gesture],
            output_path=output_dir,
            filename_prefix=f"InteractiveGesture"
        )
        if files:
            saved_files.extend(files)
            print(f"  ✓ Saved: {files[0]}")
    
    # Convert all recorded gestures
    print("\n[Step] Converting all recorded gestures to H5...")
    gen_h5(
        json_files=saved_files,
        h5_file="./output/interactive_gestures.hdf5",
        sort_by='time',  # Sort by recording time
        interval=1.0
    )
    
    print("\n✓ Interactive workflow finished!")


# ============================================================================
# Example 8: Grouped Gesture Workflow with Different Intervals and Repetitions
# ============================================================================

def example_grouped_gesture_workflow(txt_file_path: str = "./example_gesture_data.txt"):
    """
    Grouped gesture workflow: Process gestures in groups with different intervals and repetitions
    
    Configuration:
    - First 3 gestures: interval 2s, repeat 3 times
    - Middle 3 gestures: interval 3s, repeat 2 times
    - Last 3 gestures: interval 3s, repeat 5 times
    
    Args:
        txt_file_path: Path to gesture data file (default: ./example_gesture_data.txt)
    """
    print("=" * 60)
    print("Example 8: Grouped Gesture Workflow")
    print("=" * 60)
    
    if not os.path.exists(txt_file_path):
        print(f"Error: File not found: {txt_file_path}")
        return
    
    # Step 1: Parse external file
    print(f"\n[Step 1] Parsing gesture data file: {txt_file_path}")
    gestures = parse_txt_file_to_gestures(txt_file_path)
    
    if not gestures:
        print("Error: No valid gestures found in file")
        return
    
    print(f"✓ Found {len(gestures)} gesture(s)")
    
    if len(gestures) < 9:
        print(f"Warning: Expected at least 9 gestures, found {len(gestures)}")
        print("Will process available gestures in groups")
    
    # Step 2: Save as JSON files
    output_dir = "./exports"
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"\n[Step 2] Saving gestures as JSON files...")
    saved_files = save_gestures_from_array(
        gesture_array=gestures,
        output_path=output_dir,
        filename_prefix="GroupedGesture"
    )
    
    if not saved_files:
        print("Failed to save gesture files")
        return
    
    print(f"✓ Saved {len(saved_files)} JSON files")
    
    # Step 3: Group gestures and convert to H5 with different settings
    print("\n[Step 3] Converting gestures to H5 with grouped settings...")
    h5_file = "./output/grouped_gestures.hdf5"
    
    # Remove existing H5 file if it exists
    if os.path.exists(h5_file):
        os.remove(h5_file)
        print(f"  Removed existing H5 file: {h5_file}")
    
    # Group 1: First 3 gestures - interval 2s, repeat 3 times
    if len(saved_files) >= 3:
        print("\n  [Group 1] Processing first 3 gestures (interval: 2s, repeat: 3 times)...")
        group1_files = saved_files[:3]
        gen_h5(
            json_files=group1_files,
            h5_file=h5_file,
            interval=2,
            interval_unit='s',
            repeat_count=5,
            sort_by='number',
            append=False  # First group creates the file
        )
        print(f"  ✓ Group 1: Added {len(group1_files)} gestures (repeated 3 times)")
    
    # Group 2: Middle 3 gestures - interval 3s, repeat 2 times
    if len(saved_files) >= 6:
        print("\n  [Group 2] Processing middle 3 gestures (interval: 3s, repeat: 2 times)...")
        group2_files = saved_files[3:6]
        gen_h5(
            json_files=group2_files,
            h5_file=h5_file,
            interval=3,
            interval_unit='s',
            repeat_count=2,
            sort_by='number',
            append=True  # Append to existing file
        )
        print(f"  ✓ Group 2: Added {len(group2_files)} gestures (repeated 2 times)")
    
    # Group 3: Last 3 gestures - interval 3s, repeat 5 times
    if len(saved_files) >= 9:
        print("\n  [Group 3] Processing last 3 gestures (interval: 3s, repeat: 5 times)...")
        group3_files = saved_files[6:9]
        gen_h5(
            json_files=group3_files,
            h5_file=h5_file,
            interval=3,
            interval_unit='s',
            repeat_count=5,
            sort_by='number',
            append=True  # Append to existing file
        )
        print(f"  ✓ Group 3: Added {len(group3_files)} gestures (repeated 5 times)")
    
    # Verify H5 file exists
    if not os.path.exists(h5_file):
        print(f"Error: H5 file not created: {h5_file}")
        return
    
    print(f"\n✓ H5 file created: {h5_file}")
    print("\nSummary:")
    if len(saved_files) >= 3:
        print(f"  Group 1: {len(saved_files[:3])} gestures × 3 repeats = {len(saved_files[:3]) * 3} total")
    if len(saved_files) >= 6:
        print(f"  Group 2: {len(saved_files[3:6])} gestures × 2 repeats = {len(saved_files[3:6]) * 2} total")
    if len(saved_files) >= 9:
        print(f"  Group 3: {len(saved_files[6:9])} gestures × 5 repeats = {len(saved_files[6:9]) * 5} total")
    
    # Step 4: Replay the generated H5 file
    if not REPLAY_AVAILABLE:
        print("\n⚠ Warning: H5Replay not available. Skipping replay step.")
        print("✓ Grouped gesture workflow finished!")
        return
    
    print("\n[Step 4] Connecting to device for replay...")
    print("Note: This requires a connected Sharpa device")
    
    try:
        # Get device manager
        if SharpaDeviceManager is None:
            print("Error: SharpaDeviceManager not available")
            print("✓ Grouped gesture workflow finished! (H5 file created, but replay skipped)")
            return
        
        manager = SharpaDeviceManager.get_instance()
        
        # Wait for device discovery
        print("Waiting for device discovery (this may take a few seconds)...")
        import time
        discovery_timeout = 10.0
        max_wait_time = discovery_timeout
        check_interval = 0.5
        waited_time = 0.0
        
        while waited_time < max_wait_time:
            device_sns = manager.get_all_device_sn()
            if device_sns:
                print(f"✓ Found {len(device_sns)} device(s): {', '.join(device_sns)}")
                break
            time.sleep(check_interval)
            waited_time += check_interval
            print(f"  Waiting... ({waited_time:.1f}s / {max_wait_time}s)", end='\r')
        
        print()  # New line after progress
        
        device_sns = manager.get_all_device_sn()
        if not device_sns:
            print(f"⚠ Warning: No devices found after {max_wait_time}s.")
            print("  The H5 file has been created and can be replayed later.")
            print(f"  H5 file location: {h5_file}")
            print("✓ Grouped gesture workflow finished!")
            return
        
        # Connect to device (try RIGHT hand first, then LEFT)
        hand = None
        for hand_side in [HandSide.RIGHT, HandSide.LEFT]:
            try:
                print(f"\nAttempting to connect to {hand_side.name} hand...")
                hand = manager.connect(hand_side)
                if hand:
                    break
            except RuntimeError as e:
                print(f"  Could not connect to {hand_side.name} hand: {e}")
                continue
        
        if hand is None:
            print(f"Error: Could not connect to any hand device")
            print("  The H5 file has been created and can be replayed later.")
            print(f"  H5 file location: {h5_file}")
            print("✓ Grouped gesture workflow finished!")
            return
        
        print(f"✓ Connected to device: {hand.get_device_info().sn}")
        
        # Set control mode to POSITION before replay
        print("\n[Step 4.5] Setting control mode to POSITION...")
        try:
            from sharpa import ControlMode
            error = hand.set_control_mode(ControlMode.POSITION)
            if error.code != 0:
                print(f"  ⚠ Warning: Failed to set control mode: {error.message}")
            else:
                print("  ✓ Control mode set to POSITION")
        except Exception as e:
            print(f"  ⚠ Warning: Could not set control mode: {e}")
        
        # Create mock service for H5Replay
        class MockHandService:
            def __init__(self, hand_device):
                self.hs_hand = hand_device
                self.device_info = hand_device.get_device_info()
                self.call_count = 0
            
            def set_angles(self, angles, source, direct_control=False):
                try:
                    self.call_count += 1
                    if self.call_count % 10 == 0:  # Print every 10th call
                        print(f"  → Setting angles (call #{self.call_count}, angles: {len(angles)} joints)")
                    error = self.hs_hand.set_joint_position(angles)
                    if error.code != 0:
                        print(f"  ✗ Error setting angles at call #{self.call_count}: [{error.code}] {error.message}")
                    return error.code == 0
                except Exception as e:
                    print(f"  ✗ Exception setting angles at call #{self.call_count}: {e}")
                    import traceback
                    traceback.print_exc()
                    return False
        
        mock_service = MockHandService(hand)
        
        # Create H5Replay instance
        replay = H5Replay(mock_service)
        
        # Start replay
        print(f"\n[Step 6] Starting replay from: {h5_file}")
        print("  Press Ctrl+C to stop replay")
        
        replay.start(h5_file)
        
        # Wait for replay completion
        wait_for_replay_completion(replay, "Step 7")
        
        print("\n✓ Grouped gesture workflow with replay finished!")
        
    except Exception as e:
        print(f"\n⚠ Warning: Replay failed: {e}")
        print("  The H5 file has been created and can be replayed later.")
        print(f"  H5 file location: {h5_file}")
        import traceback
        traceback.print_exc()
        print("\n✓ Grouped gesture workflow finished!")
    


# ============================================================================
# Example 9: Complete Workflow with Replay
# ============================================================================

def example_complete_workflow_with_replay(discovery_timeout=10.0):
    """
    Complete workflow: Generate H5 file and replay it on device
    
    Args:
        discovery_timeout: Maximum time to wait for device discovery (seconds, default: 10.0)
    """
    if not REPLAY_AVAILABLE:
        print("Error: H5Replay not available. Cannot run replay example.")
        return
    
    print("=" * 60)
    print("Example 9: Complete Workflow with Replay")
    print("=" * 60)
    
    output_dir = "./exports"
    h5_output_dir = "./output"
    os.makedirs(output_dir, exist_ok=True)
    os.makedirs(h5_output_dir, exist_ok=True)
    
    # Step 1: Generate JSON files from predefined data
    print("\n[Step 1] Generating JSON files from predefined data...")
    if not GESTURE_DATA:
        print("Error: GESTURE_DATA is empty")
        return
    
    saved_files = save_gestures_from_array(
        gesture_array=GESTURE_DATA,
        output_path=output_dir,
        filename_prefix="ReplayGesture"
    )
    
    if not saved_files:
        print("Failed to save gesture files")
        return
    
    print(f"✓ Generated {len(saved_files)} JSON files")
    
    # Step 2: Convert to H5
    print("\n[Step 2] Converting JSON files to H5...")
    h5_file = os.path.join(h5_output_dir, "replay_gestures.hdf5")
    
    gen_h5(
        json_files=saved_files,
        h5_file=h5_file,
        sort_by='number',
        interval=1.0,
        interval_unit='s'
    )
    
    # Verify H5 file exists
    if not os.path.exists(h5_file):
        print(f"Error: H5 file not created: {h5_file}")
        return
    
    print(f"✓ H5 file created: {h5_file}")
    
    # Step 3: Connect to device and replay
    print("\n[Step 3] Connecting to device for replay...")
    print("Note: This requires a connected Sharpa device")
    
    try:
        # Get device manager
        if SharpaDeviceManager is None:
            print("Error: SharpaDeviceManager not available")
            return
        manager = SharpaDeviceManager.get_instance()
        
        # Wait for device discovery (devices need time to send heartbeat packets)
        print("Waiting for device discovery (this may take a few seconds)...")
        import time
        max_wait_time = discovery_timeout  # Maximum wait time in seconds
        check_interval = 0.5  # Check every 0.5 seconds
        waited_time = 0.0
        
        while waited_time < max_wait_time:
            device_sns = manager.get_all_device_sn()
            if device_sns:
                print(f"✓ Found {len(device_sns)} device(s): {', '.join(device_sns)}")
                break
            time.sleep(check_interval)
            waited_time += check_interval
            print(f"  Waiting... ({waited_time:.1f}s / {max_wait_time}s)", end='\r')
        
        print()  # New line after progress
        
        # Check if any devices were found
        device_sns = manager.get_all_device_sn()
        if not device_sns:
            print(f"⚠ Warning: No devices found after {max_wait_time}s. Possible reasons:")
            print("  - Device is not powered on")
            print("  - Device is not connected to the network")
            print("  - Firewall blocking UDP port 54321")
            print("  - Device discovery timeout too short")
            print("\n  The H5 file has been created and can be replayed later.")
            print(f"  H5 file location: {h5_file}")
            return
        
        # Try to connect to device
        print("Attempting to connect to RIGHT hand...")
        try:
            hand = manager.connect(HandSide.RIGHT)
        except RuntimeError as e:
            # If connection fails, try to get more info
            all_devices = manager.get_all_devices()
            if all_devices:
                print(f"  Available devices:")
                for dev in all_devices:
                    print(f"    - SN: {dev.sn}, Side: {dev.hand_side}, Type: {dev.device_type}")
            raise e
        
        if hand is None:
            print("⚠ Warning: Could not connect to device. Skipping replay.")
            print("  The H5 file has been created and can be replayed later.")
            print(f"  H5 file location: {h5_file}")
            return
        
        print(f"✓ Connected to device: {hand.get_device_info().sn}")
        
        # Set control mode to POSITION before replay
        print("\n[Step 4] Setting control mode to POSITION...")
        try:
            from sharpa import ControlMode
            error = hand.set_control_mode(ControlMode.POSITION)
            if error.code != 0:
                print(f"  ⚠ Warning: Failed to set control mode: {error.message}")
            else:
                print("  ✓ Control mode set to POSITION")
        except Exception as e:
            print(f"  ⚠ Warning: Could not set control mode: {e}")
        
        # Create a mock HandService for replay
        # Note: In real usage, this would come from HandManager
        print("\n[Step 5] Setting up replay...")
        
        # Create a simple service-like object with set_angles method
        class MockHandService:
            def __init__(self, hand_device):
                self.hs_hand = hand_device
                self.device_info = hand_device.get_device_info()
                self.call_count = 0
            
            def set_angles(self, angles, source, direct_control=False):
                """Set joint angles on the device"""
                try:
                    self.call_count += 1
                    if self.call_count % 10 == 0:  # Print every 10th call
                        print(f"  → Setting angles (call #{self.call_count}, angles: {len(angles)} joints)")
                    error = self.hs_hand.set_joint_position(angles)
                    if error.code != 0:
                        print(f"  ⚠ Warning: set_angles error at call #{self.call_count}: [{error.code}] {error.message}")
                    return error.code == 0
                except Exception as e:
                    print(f"  ✗ Error setting angles at call #{self.call_count}: {e}")
                    import traceback
                    traceback.print_exc()
                    return False
        
        mock_service = MockHandService(hand)
        
        # Create H5Replay instance
        replay = H5Replay(mock_service)
        
        # Start replay
        print(f"\n[Step 5] Starting replay from: {h5_file}")
        print("  Press Ctrl+C to stop replay")
        
        replay.start(h5_file)
        
        # Wait for replay completion
        wait_for_replay_completion(replay, "Step 5")
        
        print("\n✓ Complete workflow with replay finished!")
        
    except Exception as e:
        print(f"\n⚠ Warning: Replay failed: {e}")
        print("  The H5 file has been created and can be replayed later.")
        print(f"  H5 file location: {h5_file}")
        import traceback
        traceback.print_exc()


# ============================================================================
# Example 9: Replay Existing H5 File
# ============================================================================

def example_replay_existing_h5(h5_file_path: str, hand_side: HandSide = HandSide.RIGHT, discovery_timeout=10.0):
    """
    Replay an existing H5 file on device
    
    Args:
        h5_file_path: Path to H5 file to replay
        hand_side: Hand side to connect to (LEFT or RIGHT)
        discovery_timeout: Maximum time to wait for device discovery (seconds, default: 10.0)
    """
    if not REPLAY_AVAILABLE:
        print("Error: H5Replay not available. Cannot run replay example.")
        return
    
    print("=" * 60)
    print("Example 9: Replay Existing H5 File")
    print("=" * 60)
    
    if not os.path.exists(h5_file_path):
        print(f"Error: H5 file not found: {h5_file_path}")
        return
    
    print(f"\nH5 file: {h5_file_path}")
    print(f"Hand side: {hand_side.name}")
    
    try:
        # Connect to device
        print("\n[Step 1] Connecting to device...")
        if SharpaDeviceManager is None:
            print("Error: SharpaDeviceManager not available")
            return
        manager = SharpaDeviceManager.get_instance()
        
        # Wait for device discovery
        print("Waiting for device discovery (this may take a few seconds)...")
        import time
        max_wait_time = discovery_timeout  # Maximum wait time in seconds
        check_interval = 0.5  # Check every 0.5 seconds
        waited_time = 0.0
        
        while waited_time < max_wait_time:
            device_sns = manager.get_all_device_sn()
            if device_sns:
                print(f"✓ Found {len(device_sns)} device(s): {', '.join(device_sns)}")
                break
            time.sleep(check_interval)
            waited_time += check_interval
            print(f"  Waiting... ({waited_time:.1f}s / {max_wait_time}s)", end='\r')
        
        print()  # New line after progress
        
        # Check if any devices were found
        device_sns = manager.get_all_device_sn()
        if not device_sns:
            print(f"⚠ Warning: No devices found after {max_wait_time}s.")
            print("  Please check device connection and network settings.")
            return
        
        # Try to connect
        try:
            hand = manager.connect(hand_side)
        except RuntimeError as e:
            # If connection fails, show available devices
            all_devices = manager.get_all_devices()
            if all_devices:
                print(f"  Available devices:")
                for dev in all_devices:
                    print(f"    - SN: {dev.sn}, Side: {dev.hand_side}, Type: {dev.device_type}")
            raise e
        
        if hand is None:
            print(f"Error: Could not connect to {hand_side.name} hand device")
            return
        
        print(f"✓ Connected to device: {hand.get_device_info().sn}")
        
        # Set control mode to POSITION before replay
        print("\n[Step 1.5] Setting control mode to POSITION...")
        try:
            from sharpa import ControlMode
            error = hand.set_control_mode(ControlMode.POSITION)
            if error.code != 0:
                print(f"  ⚠ Warning: Failed to set control mode: {error.message}")
            else:
                print("  ✓ Control mode set to POSITION")
        except Exception as e:
            print(f"  ⚠ Warning: Could not set control mode: {e}")
        
        # Create mock service
        class MockHandService:
            def __init__(self, hand_device):
                self.hs_hand = hand_device
                self.device_info = hand_device.get_device_info()
                self.call_count = 0
            
            def set_angles(self, angles, source, direct_control=False):
                try:
                    self.call_count += 1
                    if self.call_count % 10 == 0:  # Print every 10th call
                        print(f"  → Setting angles (call #{self.call_count}, angles: {len(angles)} joints)")
                    error = self.hs_hand.set_joint_position(angles)
                    if error.code != 0:
                        print(f"  ✗ Error setting angles at call #{self.call_count}: [{error.code}] {error.message}")
                    return error.code == 0
                except Exception as e:
                    print(f"  ✗ Exception setting angles at call #{self.call_count}: {e}")
                    import traceback
                    traceback.print_exc()
                    return False
        
        mock_service = MockHandService(hand)
        
        # Create and start replay
        print("\n[Step 2] Starting replay...")
        print("  Press Ctrl+C to stop replay")
        
        replay = H5Replay(mock_service)
        replay.start(h5_file_path)
        
        # Wait for replay completion
        wait_for_replay_completion(replay, "Step 3")
        
    except Exception as e:
        print(f"\n✗ Replay failed: {e}")
        import traceback
        traceback.print_exc()


# ============================================================================
# Main Function
# ============================================================================

def main():
    """Main function to run examples"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Complete Gesture Workflow Examples',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Record from device and convert to H5
  python gesture_workflow_example.py --workflow record
  
  # Use predefined data
  python gesture_workflow_example.py --workflow predefined
  
  # Read from external file
  python gesture_workflow_example.py --workflow external --input-file ./gesture_data.txt
  
  # Advanced workflow
  python gesture_workflow_example.py --workflow advanced
  
  # Batch process
  python gesture_workflow_example.py --workflow batch
  
  # Complete pipeline with error handling
  python gesture_workflow_example.py --workflow pipeline
  
  # Interactive workflow
  python gesture_workflow_example.py --workflow interactive
  
  # Grouped gesture workflow
  python gesture_workflow_example.py --workflow grouped --gesture-file ./example_gesture_data.txt
  
  # Generate and replay
  python gesture_workflow_example.py --workflow replay
  
  # Replay existing H5 file
  python gesture_workflow_example.py --workflow replay-existing --h5-file ./output/replay_gestures.hdf5
  
  # Run all workflows
  python gesture_workflow_example.py --all
        """
    )
    
    parser.add_argument(
        '--workflow',
        type=str,
        choices=['record', 'predefined', 'external', 'advanced', 'batch', 'pipeline', 'interactive', 'grouped', 'replay', 'replay-existing'],
        help='Workflow type to run: record, predefined, external, advanced, batch, pipeline, interactive, grouped, replay, replay-existing'
    )
    
    parser.add_argument(
        '--gesture-file',
        type=str,
        default='./example_gesture_data.txt',
        help='Path to gesture data file (for grouped workflow, default: ./example_gesture_data.txt)'
    )
    
    parser.add_argument(
        '--all',
        action='store_true',
        help='Run all examples'
    )
    
    parser.add_argument(
        '--input-file',
        type=str,
        help='Input file path (for external workflow)'
    )
    
    parser.add_argument(
        '--h5-file',
        type=str,
        help='H5 file path to replay (for replay-existing workflow)'
    )
    
    parser.add_argument(
        '--hand-side',
        type=str,
        choices=['LEFT', 'RIGHT'],
        default='RIGHT',
        help='Hand side for replay (for grouped and replay-existing workflows, default: RIGHT)'
    )
    
    parser.add_argument(
        '--discovery-timeout',
        type=float,
        default=10.0,
        help='Maximum time to wait for device discovery in seconds (default: 10.0)'
    )
    
    args = parser.parse_args()
    
    if args.all:
        # Run all examples
        workflows = [
            ('predefined', example_predefined_data_to_h5),
            ('advanced', example_advanced_workflow),
            ('batch', example_batch_process),
            ('pipeline', example_complete_pipeline_with_error_handling),
            ('interactive', example_interactive_workflow),
            # Skip record, grouped, replay, replay-existing (require device connection)
        ]
        
        for name, func in workflows:
            try:
                print(f"\n\n{'='*60}")
                print(f"Running Workflow: {name}")
                print('='*60)
                func()
            except Exception as e:
                print(f"\n✗ Workflow '{name}' failed: {e}")
                import traceback
                traceback.print_exc()
        
        print("\n\n" + "="*60)
        print("All workflows completed!")
        print("="*60)
        
    elif args.workflow:
        # Run specific workflow
        hand_side = HandSide.LEFT if args.hand_side == 'LEFT' else HandSide.RIGHT
        
        workflows = {
            'record': example_record_from_device_to_h5,
            'predefined': example_predefined_data_to_h5,
            'external': lambda: example_external_file_to_h5(args.input_file or "./example_gesture_data.txt"),
            'advanced': example_advanced_workflow,
            'batch': example_batch_process,
            'pipeline': example_complete_pipeline_with_error_handling,
            'interactive': example_interactive_workflow,
            'grouped': lambda: example_grouped_gesture_workflow(args.gesture_file),
            'replay': lambda: example_complete_workflow_with_replay(args.discovery_timeout),
            'replay-existing': lambda: example_replay_existing_h5(
                args.h5_file or "./output/replay_gestures.hdf5",
                hand_side,
                args.discovery_timeout
            ),
        }
        
        if args.workflow in workflows:
            try:
                workflows[args.workflow]()
            except Exception as e:
                print(f"\n✗ Workflow '{args.workflow}' failed: {e}")
                import traceback
                traceback.print_exc()
        else:
            print(f"Unknown workflow: {args.workflow}")
    else:
        # Show menu
        print("Complete Gesture Workflow Examples")
        print("=" * 60)
        print("\nAvailable workflows:")
        print("  record              - Record from Device and Convert to H5")
        print("  predefined          - Use Predefined Data to H5")
        print("  external            - Read from External File and Convert")
        print("  advanced            - Advanced Workflow (Select, Sort, Repeat, Append)")
        print("  batch               - Batch Process Multiple Gestures")
        print("  pipeline            - Complete Pipeline with Error Handling")
        print("  interactive         - Interactive Workflow")
        print("  grouped             - Grouped Gesture Workflow (Different intervals and repetitions)")
        print("  replay              - Complete Workflow with Replay (Generate + Replay)")
        print("  replay-existing     - Replay Existing H5 File")
        print("\nUsage:")
        print("  python gesture_workflow_example.py --workflow <workflow-name>")
        print("  python gesture_workflow_example.py --all")
        print("\nOptions:")
        print("  --input-file <path>     Input file for 'external' workflow")
        print("  --gesture-file <path>   Gesture data file for 'grouped' workflow (default: ./example_gesture_data.txt)")
        print("  --h5-file <path>       H5 file to replay for 'replay-existing' workflow")
        print("  --hand-side <LEFT|RIGHT> Hand side for 'grouped' and 'replay-existing' workflows (default: RIGHT)")
        print("  --discovery-timeout <seconds> Maximum time to wait for device discovery (default: 10.0)")
        print("\nExamples:")
        print("  # Generate and replay")
        print("  python gesture_workflow_example.py --workflow grouped")
        print("  # Replay existing H5 file")
        print("  python gesture_workflow_example.py --workflow replay-existing --h5-file ./output/replay_gestures.hdf5")


if __name__ == '__main__':
    main()
