/**
 * For reference: LoopA's big-brother, V4 live forwarding handling (by TK.)
 *
 */

static s32 SEQ_LIVE_PlayEventInternal(u8 track, seq_layer_evnt_t e, u8 original_note, u32 bpm_tick)
{
   seq_core_trk_t *t = &seq_core_trk[track];
   seq_cc_trk_t *tcc = &seq_cc_trk[track];

   // transpose if no transposer/arpeggiator enabled (to avoid conflicts)
   if (tcc->playmode != SEQ_CORE_TRKMODE_Transpose && tcc->playmode != SEQ_CORE_TRKMODE_Arpeggiator)
   {
      SEQ_CORE_Transpose(track, ui_selected_instrument, t, tcc, &e.midi_package);
   }

   //initialize robotize flags
   seq_robotize_flags_t robotize_flags;
   robotize_flags.ALL = 0;

   // adding FX?
   u8 apply_force_to_scale = tcc->event_mode != SEQ_EVENT_MODE_Drum;
   if (seq_live_options.FX)
   {
      SEQ_HUMANIZE_Event(track, t->step, &e);
      robotize_flags = SEQ_ROBOTIZE_Event(track, t->step, &e);
      if (!robotize_flags.NOFX)
         SEQ_LFO_Event(track, &e);

      if (apply_force_to_scale &&
          (seq_live_options.FORCE_SCALE
      )
              )
      {
         u8 scale, root_selection, root;
         SEQ_CORE_FTS_GetScaleAndRoot(track, t->step, ui_selected_instrument, tcc, &scale, &root_selection, &root);
         SEQ_SCALE_Note(&e.midi_package, scale, root);
      }
      SEQ_CORE_Limit(t, tcc, &e); // should be the last Fx in the chain!

   } else
   {
      if (apply_force_to_scale && (seq_live_options.FORCE_SCALE ))
      {
         u8 scale, root_selection, root;
         SEQ_CORE_FTS_GetScaleAndRoot(track, t->step, ui_selected_instrument, tcc, &scale, &root_selection, &root);
         SEQ_SCALE_Note(&e.midi_package, scale, root);
      }
   }

   if (original_note)
   {
      // note value may have been changed by FTS, Limit or Humanizer - capture it in live_keyboard_note
      live_keyboard_note[original_note] = e.midi_package.note;
   }

   if (bpm_tick == 0xffffffff)
   {
      mios32_midi_port_t port = tcc->midi_port;
      MUTEX_MIDIOUT_TAKE;
      SEQ_MIDI_PORT_FilterOscPacketsSet(1); // important to avoid OSC feedback loops!
      MIOS32_MIDI_SendPackage(port, e.midi_package);
      SEQ_MIDI_PORT_FilterOscPacketsSet(0);
      MUTEX_MIDIOUT_GIVE;
   }
   else
   {
      // Note On (the Note Off will be prepared as well in SEQ_CORE_ScheduleEvent)
      u32 scheduled_tick = bpm_tick + t->bpm_tick_delay;
      SEQ_CORE_ScheduleEvent(track, t, tcc, e.midi_package, SEQ_MIDI_OUT_OnOffEvent, scheduled_tick, e.len, 0,
                             robotize_flags);
   }


   // adding echo?
   if (seq_live_options.FX)
   {
      if (SEQ_BPM_IsRunning())
         if (!robotize_flags.NOFX)
            SEQ_CORE_Echo(track, ui_selected_instrument, t, tcc, e.midi_package,
                          (bpm_tick == 0xffffffff) ? SEQ_BPM_TickGet() : bpm_tick, e.len, robotize_flags);
   }

   return 0; // no error
}

s32 SEQ_LIVE_PlayEvent(u8 track, mios32_midi_package_t p)
{
   mios32_midi_port_t port = seq_cc_trk[track].midi_port;
   u8 chn = seq_live_options.KEEP_CHANNEL ? p.chn : seq_cc_trk[track].midi_chn;

   // temporary mute matching events from the sequencer
   SEQ_CORE_NotifyIncomingMIDIEvent(track, p);

   // Note Events:
   if (p.type == NoteOff)
   {
      p.type = NoteOn;
      p.velocity = 0;
   }

   if (p.type == NoteOn)
   {
      u32 note_ix32 = p.note / 32;
      u32 note_mask = (1 << (p.note % 32));

      // in any case (key depressed or not), play note off if note is active!
      // this ensures that note off event is sent if for example the OCT_TRANSPOSE has been changed
      if (seq_live_played_notes[note_ix32] & note_mask)
      {
         // send velocity off
         MUTEX_MIDIOUT_TAKE;
         SEQ_MIDI_PORT_FilterOscPacketsSet(1); // important to avoid OSC feedback loops!
         MIOS32_MIDI_SendNoteOn(live_keyboard_port[p.note],
                                live_keyboard_chn[p.note],
                                live_keyboard_note[p.note],
                                0x00);
         SEQ_MIDI_PORT_FilterOscPacketsSet(0);
         MUTEX_MIDIOUT_GIVE;
      }

      if (p.velocity == 0)
      {
         seq_live_played_notes[note_ix32] &= ~note_mask;
      }
      else
      {
         seq_live_played_notes[note_ix32] |= note_mask;

         int effective_note;
         u8 event_mode = SEQ_CC_Get(track, SEQ_CC_MIDI_EVENT_MODE);
         if (event_mode == SEQ_EVENT_MODE_Drum)
         {
            effective_note = (int) p.note; // transpose disabled in UI
         }
         else
         {
            effective_note = (int) p.note + 12 * seq_live_options.OCT_TRANSPOSE;

            // ensure that note is in the 0..127 range
            effective_note = SEQ_CORE_TrimNote(effective_note, 0, 127);
         }

         // Track Repeat function:
         u8 play_note = 1;
         if (track == SEQ_UI_VisibleTrackGet())
         {
               seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t * ) & seq_live_pattern_slot[0];
               // take over note value and velocity
               slot->chn = chn;
               slot->note = effective_note;
               slot->velocity = p.velocity;

               // don't play note immediately if repeat function enabled and sequencer is running
               if (slot->enabled && SEQ_BPM_IsRunning())
                  play_note = 0;
         }

         live_keyboard_port[p.note] = port;
         live_keyboard_chn[p.note] = chn;
         live_keyboard_note[p.note] = effective_note;

         if (play_note)
         {
            seq_layer_evnt_t e;
            e.midi_package = p;
            e.midi_package.chn = chn;
            e.midi_package.note = effective_note;
            e.len = 95; // full note (only used for echo effects)
            e.layer_tag = 0;

            SEQ_LIVE_PlayEventInternal(track, e, p.note, 0xffffffff);
         }
      }
   }
   else if (p.type >= 0x8 && p.type <= 0xe)
   {

      // just forward event over right channel...
      p.chn = chn;
      MUTEX_MIDIOUT_TAKE;
      SEQ_MIDI_PORT_FilterOscPacketsSet(1); // important to avoid OSC feedback loops!
      MIOS32_MIDI_SendPackage(port, p);
      SEQ_MIDI_PORT_FilterOscPacketsSet(0);
      MUTEX_MIDIOUT_GIVE;

      // Aftertouch: take over velocity in repeat slot
      if (p.type == PolyPressure)
      {
         int i;
         seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t * ) & seq_live_pattern_slot[0];
         for (i = 0; i < SEQ_LIVE_PATTERN_SLOTS; ++i, ++slot)
         {
            if (slot->note == p.note)
               slot->velocity = p.velocity;
         }
      } else if (p.type == Aftertouch)
      {
         int i;
         seq_live_pattern_slot_t *slot = (seq_live_pattern_slot_t * ) & seq_live_pattern_slot[0];
         for (i = 0; i < SEQ_LIVE_PATTERN_SLOTS; ++i, ++slot)
         {
            slot->velocity = p.evnt1;
         }
      }
   }

   return 0; // no error
}
