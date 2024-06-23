
  /*
  #ifndef EEPROM_ENABLED
  String uuid("ND");
#endif

  The following is kept for reference, and future dev
  if( EEPROM.length() == 0 )
  {
    #ifdef DEBUG
      DEBUG_DEV.println("EEPROM.length() reported zero bytes, setting to default of 1024 using .begin()..");
    #endif
    EEPROM.begin(EEPROM_DEFAULT_SIZE);
  }


  #ifdef DEBUG
    DEBUG_DEV.print("EEPROM length: ");
    DEBUG_DEV.println(EEPROM.length());
  #endif


  if( EEPROM.length() == 0 )
  {
      #ifdef DEBUG
        DEBUG_DEV.println("Error. EEPROM.length() reported zero bytes.");
      #endif
      while (true)
      {
        do_blink_sequence(1, 250, 250);
      }

  }

  EEPROM.get(EEPROM_PROVISIONING_ADDRESS, epd);

  if(epd.header != EEPROM_HEADER)
  {
    #ifdef DEBUG
      DEBUG_DEV.println("EEPROM HEADER MISSING, GENERATING NEW UID..");
    #endif

    uuid.generate();
    char u[9];

    // No reason to use all 37 characters of the generated UID. Instead,
    // we can just use the first 8 characters, which is unique enough to ensure
    // a user will likely never generate a duplicate UID unless they have thousands
    // of arduinos.
    memcpy( (void*)epd.uid, uuid.toCharArray(), 8);
    epd.uid[8] = 0;
    epd.header = EEPROM_HEADER;
    epd.configLen = 0;
    epd.configVersion = EEPROM_CONFIG_FORMAT_VERSION;
    epd.configCRC = 0;
      
    EEPROM.put(EEPROM_PROVISIONING_ADDRESS, epd);
    EEPROM.commit();

    serialClient.setUID(epd.uid);
    
    
    #ifdef DEBUG
    DEBUG_DEV.print("Wrote header value = 0x");
    DEBUG_DEV.print(epd.header, HEX);
    DEBUG_DEV.print(" to EEPROM and uid value = ");
    DEBUG_DEV.print((char*)epd.uid);
    DEBUG_DEV.print(" to EEPROM.");
    #endif
    
  }
  else
  {
    #ifdef DEBUG
      DEBUG_DEV.println("\nEEPROM HEADER DUMP");
      DEBUG_DEV.print("Head = 0x");
      DEBUG_DEV.println(epd.header, HEX);
      DEBUG_DEV.print("Config Version = 0x");
      DEBUG_DEV.println(epd.configVersion, HEX);
    #endif

    if(epd.configVersion != EEPROM_CONFIG_FORMAT_VERSION)
    {
      #ifdef DEBUG
        DEBUG_DEV.print("Error. Expected EEPROM Config Version: 0x");
        DEBUG_DEV.print(EEPROM_CONFIG_FORMAT_VERSION);
        DEBUG_DEV.print(", got: 0x");
        DEBUG_DEV.println(epd.configVersion);
      #endif

      while (true)
      {
        do_blink_sequence(1, 250, 250);
      // Loop forver as the expected config version does not match!
      }
    }
    #ifdef DEBUG
      DEBUG_DEV.print("Config Length = 0x");
      DEBUG_DEV.println(epd.configLen, HEX);
      DEBUG_DEV.print("Config CRC = 0x");
      DEBUG_DEV.println(epd.configCRC, HEX);
    #endif

    serialClient.setUID(epd.uid);
   
  }

  #ifndef EEPROM_ENABLED
    //String uuid("ND");
    serialClient.setUID(uuid.c_str());
  #endif

  #ifdef ENABLE_RAPIDCHANGE
    rc_setup();
  #endif

    */