#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <string.h>

class RingBuffer
{
public:
	RingBuffer( void* buffer, unsigned int buffer_size )
	{
        m_buff = (unsigned char*)buffer;
		m_buff_end = m_buff + buffer_size;
		m_load_ptr = m_consume_ptr = m_buff;
		m_max_load = buffer_size;
		m_max_consume = m_data_in_buffer = 0;
		Update_State();
	}
    
	// Try to add data to the buffer. After the call, 'len' contains
	// the amount of bytes actually buffered.
	void AddData( const void* data, unsigned int& len )
	{
		if ( len > (unsigned int)m_max_load )
			len = (unsigned int)m_max_load;
		memcpy( m_load_ptr, data, len );
		m_load_ptr += len;
		m_data_in_buffer += len;
		Update_State();
	}
    
	// Request 'len' bytes from the buffer. After the call,
	// 'len' contains the amount of bytes actually copied.
	void GetData( void* outData, unsigned int& len )
	{
		if ( len > (unsigned int)m_max_consume )
			len = (unsigned int)m_max_consume;
		memcpy( outData, m_consume_ptr, len );
		m_consume_ptr += len;
		m_data_in_buffer -= len;
		Update_State();
	}
    
	// Tries to skip len bytes. After the call,
	// 'len' contains the realized skip.
	void SkipData( unsigned int& len )
	{
		unsigned int requestedSkip = len;
		for ( int i=0; i<2; ++i ) // This may wrap  so try it twice
		{
			int skip = (int)len;
			if ( skip > m_max_consume )
				skip = m_max_consume;
			m_consume_ptr += skip;
			m_data_in_buffer -= skip;
			len -= skip;
			Update_State();
		}
		len = requestedSkip - len;
	}
    
	// The amount of data the buffer can currently receive on one Buffer_Data() call.
	inline unsigned int FreeSpace()    { return (unsigned int)m_max_load; }
    
	// The total amount of data in the buffer. Note that it may not be continuous: you may need
	// two successive calls to Get_Data() to get it all.
	inline unsigned int BufferedBytes() { return (unsigned int)m_data_in_buffer; }
    
private:
    
	void Update_State()
	{
		if (m_consume_ptr == m_buff_end)	m_consume_ptr = m_buff;
		if (m_load_ptr == m_buff_end)		m_load_ptr = m_buff;
        
		if (m_load_ptr == m_consume_ptr)
		{
			if ( m_data_in_buffer > 0 )
			{
				m_max_load = 0;
				m_max_consume = m_buff_end - m_consume_ptr;
			}
			else
			{
				m_max_load = m_buff_end - m_load_ptr;
				m_max_consume = 0;
			}
		}
		else if ( m_load_ptr > m_consume_ptr )
		{
			m_max_load = m_buff_end - m_load_ptr;
			m_max_consume = m_load_ptr - m_consume_ptr;
		}
		else
		{
			m_max_load = m_consume_ptr - m_load_ptr;
			m_max_consume = m_buff_end - m_consume_ptr;
		}
	}
    
	unsigned char *m_load_ptr, *m_consume_ptr;
	unsigned char *m_buff_end;
	unsigned char *m_buff;
	int m_max_load, m_max_consume, m_data_in_buffer;
};

#endif