#include <Cabana_Parallel.hpp>
#include <Cabana_ExecutionPolicy.hpp>
#include <Cabana_AoSoA.hpp>

#include <boost/test/unit_test.hpp>

//---------------------------------------------------------------------------//
// Check the data given a set of values.
template<class aosoa_type>
void checkDataMembers(
    aosoa_type aosoa,
    const float fval, const double dval, const int ival,
    const std::size_t dim_1, const std::size_t dim_2,
    const std::size_t dim_3, const std::size_t dim_4 )
{
    for ( auto idx = aosoa.begin(); idx != aosoa.end(); ++idx )
    {
        // Member 0.
        for ( std::size_t i = 0; i < dim_1; ++i )
            for ( std::size_t j = 0; j < dim_2; ++j )
                for ( std::size_t k = 0; k < dim_3; ++k )
                    BOOST_CHECK( aosoa.template get<0>( idx, i, j, k ) ==
                                fval * (i+j+k) );

        // Member 1.
        BOOST_CHECK( aosoa.template get<1>( idx ) == ival );

        // Member 2.
        for ( std::size_t i = 0; i < dim_1; ++i )
            for ( std::size_t j = 0; j < dim_2; ++j )
                for ( std::size_t k = 0; k < dim_3; ++k )
                    for ( std::size_t l = 0; l < dim_4; ++l )
                        BOOST_CHECK( aosoa.template get<2>( idx, i, j, k, l ) ==
                                    fval * (i+j+k+l) );

        // Member 3.
        for ( std::size_t i = 0; i < dim_1; ++i )
            BOOST_CHECK( aosoa.template get<3>( idx, i ) == dval * i );
        for ( std::size_t i = 0; i < dim_1; ++i )
            printf( "%f, %f\n", aosoa.template get<3>( idx, i ),  dval * i );


        // Member 4.
        for ( std::size_t i = 0; i < dim_1; ++i )
            for ( std::size_t j = 0; j < dim_2; ++j )
                BOOST_CHECK( aosoa.template get<4>( idx, i, j ) == dval * (i+j) );
    }
}

//---------------------------------------------------------------------------//
// Assignment operator.
template<class AoSoA_t>
class AssignmentOp
{
  public:
    AssignmentOp( AoSoA_t aosoa,
                  float fval,
                  double dval,
                  int ival )
        : _aosoa( aosoa )
        , _fval( fval )
        , _dval( dval )
        , _ival( ival )
        , _dim_1( aosoa.extent(2,0) )
        , _dim_2( aosoa.extent(2,1) )
        , _dim_3( aosoa.extent(2,2) )
        , _dim_4( aosoa.extent(2,3) )
    {}

    KOKKOS_INLINE_FUNCTION void operator()( const Cabana::Index idx ) const
    {
        // Member 0.
        for ( std::size_t i = 0; i < _dim_1; ++i )
            for ( std::size_t j = 0; j < _dim_2; ++j )
                for ( std::size_t k = 0; k < _dim_3; ++k )
                    _aosoa.template get<0>( idx, i, j, k ) = _fval * (i+j+k);

        // Member 1.
        _aosoa.template get<1>( idx ) = _ival;

        // Member 2.
        for ( std::size_t i = 0; i < _dim_1; ++i )
            for ( std::size_t j = 0; j < _dim_2; ++j )
                for ( std::size_t k = 0; k < _dim_3; ++k )
                    for ( std::size_t l = 0; l < _dim_4; ++l )
                        _aosoa.template get<2>( idx, i, j, k, l ) = _fval * (i+j+k+l);

        // Member 3.
        for ( std::size_t i = 0; i < _dim_1; ++i )
            _aosoa.template get<3>( idx, i ) = _dval * i;

        // Member 4.
        for ( std::size_t i = 0; i < _dim_1; ++i )
            for ( std::size_t j = 0; j < _dim_2; ++j )
                _aosoa.template get<4>( idx, i, j ) = _dval * (i+j);
    }

  private:

    AoSoA_t _aosoa;
    float _fval;
    double _dval;
    int _ival;
    std::size_t _dim_1 = 3;
    std::size_t _dim_2 = 2;
    std::size_t _dim_3 = 4;
    std::size_t _dim_4 = 3;
};

//---------------------------------------------------------------------------//
// TESTS
//---------------------------------------------------------------------------//
BOOST_AUTO_TEST_CASE( parallel_for_test )
{
    // Data dimensions.
    const std::size_t dim_1 = 3;
    const std::size_t dim_2 = 2;
    const std::size_t dim_3 = 4;
    const std::size_t dim_4 = 3;

    // Declare data types.
    using DataTypes =
        Cabana::MemberDataTypes<float[dim_1][dim_2][dim_3],
                                int,
                                float[dim_1][dim_2][dim_3][dim_4],
                                double[dim_1],
                                double[dim_1][dim_2]
                                >;

    // Declare the AoSoA type. Let the library pick an inner array size based
    // on the execution space.
    using AoSoA_t = Cabana::AoSoA<DataTypes,TEST_MEMSPACE>;

    // Create an AoSoA.
    std::size_t num_data = 155;
    AoSoA_t aosoa( num_data );

    // Create an execution policy.
    Cabana::IndexRangePolicy<TEST_EXECSPACE>
        range_policy( aosoa.begin(), aosoa.end() );

    // Create a functor to operate on.
    float fval = 3.4;
    double dval = 1.23;
    int ival = 1;
    AssignmentOp<AoSoA_t> func_1( aosoa, fval, dval, ival );

    // Loop in parallel using 1D struct parallelism.
    Cabana::parallel_for( range_policy, func_1, Cabana::StructParallelTag() );

    // Check data members for proper initialization.
    checkDataMembers( aosoa, fval, dval, ival, dim_1, dim_2, dim_3, dim_4 );

    // Change values and write a second functor.
    fval = 93.4;
    dval = 12.1;
    ival = 4;
    AssignmentOp<AoSoA_t> func_2( aosoa, fval, dval, ival );

    // Loop in parallel using 1D array parallelism.
    Cabana::parallel_for( range_policy, func_2, Cabana::ArrayParallelTag() );

    // Check data members for proper initialization.
    checkDataMembers( aosoa, fval, dval, ival, dim_1, dim_2, dim_3, dim_4 );

    // Change values and write a third functor.
    fval = 7.7;
    dval = 3.2;
    ival = 9;
    AssignmentOp<AoSoA_t> func_3( aosoa, fval, dval, ival );

    // Loop in parallel using 2D struct and array parallelism.
    Cabana::parallel_for(
        range_policy, func_3, Cabana::StructAndArrayParallelTag() );

    // Check data members for proper initialization.
    checkDataMembers( aosoa, fval, dval, ival, dim_1, dim_2, dim_3, dim_4 );

    // Do one more loop but this time auto-dispatch. Reuse the first functor.
    Cabana::parallel_for( range_policy, func_1 );

    // Check data members for proper initialization.
    fval = 3.4;
    dval = 1.23;
    ival = 1;
    checkDataMembers( aosoa, fval, dval, ival, dim_1, dim_2, dim_3, dim_4 );
}
