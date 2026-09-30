# Video decoder fixtures

These small synthetic files contain no third-party media. Tests use the
checked-in fixtures without requiring the FFmpeg executable or encoders.

- `video_bframes.mp4`: five 32x24 H.264 frames at 5 fps, with B frames to test
  presentation order and EOF draining.
- `video_audio.mp4`: two red H.264 frames at 5 fps plus an AAC sine-wave track.
  Video timestamps start at 3 seconds, testing stream selection and preservation
  of source presentation times.
- `audio_only.wav`: an audio-only file, testing rejection of absent video streams.

Generation commands, run from the repository root:

```sh
ffmpeg -f lavfi -i testsrc2=size=32x24:rate=5:duration=1 \
  -c:v libx264 -pix_fmt yuv420p -bf 2 -g 10 -an \
  tests/fixtures/video_bframes.mp4

ffmpeg -f lavfi -i color=c=red:size=32x24:rate=5:duration=0.4 \
  -f lavfi -i sine=frequency=440:duration=0.4 \
  -c:v libx264 -pix_fmt yuv420p -c:a aac -output_ts_offset 3 -shortest \
  tests/fixtures/video_audio.mp4

ffmpeg -f lavfi -i sine=frequency=440:duration=0.1 -c:a pcm_s16le \
  tests/fixtures/audio_only.wav
```
